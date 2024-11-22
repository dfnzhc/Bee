/**
 * @File Object.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/22
 * @Brief This file is part of Bee.
 */

#include "Core/Object.hpp"
#include "Utility/Error.hpp"

using namespace bee;

#if BEE_ENABLE_REF_TRACKING

#  include <set>
#  include <iostream>
#  include <stacktrace>
#  include "Utility/Logger.hpp"
#  include "Utility/Graph/Graph.hpp"

namespace {

std::mutex sTrackedObjectsMutex;
std::set<const Object*> sTrackedObjects;

} // namespace
#endif

void Object::incRef() const
{
    uint32_t refCount = _refCount.fetch_add(1);
#if BEE_ENABLE_REF_TRACKING
    if (refCount == 0) {
        std::lock_guard<std::mutex> lock(sTrackedObjectsMutex);
        sTrackedObjects.insert(this);
    }
#endif
}

void Object::decRef(bool dealloc) const noexcept
{
    uint32_t refCount = _refCount.fetch_sub(1);
    BEE_ASSERT(refCount > 0, "无效的对象引用数量: {} {}", className(), refCount);

    if (refCount == 1) {
#if BEE_ENABLE_REF_TRACKING
        {
            std::lock_guard<std::mutex> lock(sTrackedObjectsMutex);
            sTrackedObjects.erase(this);
        }
#endif
        if (dealloc)
            delete this;
    }
}

#if BEE_ENABLE_REF_TRACKING

void Object::dumpAliveObjects()
{
    std::lock_guard<std::mutex> lock(sTrackedObjectsMutex);
    LogInfo("共 '{}' 个存活对象:", sTrackedObjects.size());
    for (const auto* object : sTrackedObjects)
        object->dumpRefs();

    DirectedGraph<std::string, u64> graph;

    for (const auto* object : sTrackedObjects) {
        auto va = graph.addVertex(std::format("{}", fmt::ptr(object)));
        for (const auto& it : object->_refTrackers) {
            auto vb = graph.addVertex(std::format("{}", it.first));
            graph.addEdge(vb, va, 1);
        }
    }

    Dump(std::cout, graph);
    std::cout << std::endl;
}

void Object::dumpRefs() const
{
    LogInfo("对象 (类为\"{}\", 地址 = {}) 存在 '{}' 个引用", className(), fmt::ptr(this), refCount());
    std::lock_guard<std::mutex> lock(_refTrackerMutex);
    for (const auto& it : _refTrackers) {
        LogInfo("\t[{}] 数量 = {}\t{}", it.first, it.second.count, it.second.origin);
    }
}

void Object::incRef(u64 refId) const
{
    if (_enableRefTracking) {
        std::lock_guard<std::mutex> lock(_refTrackerMutex);
        auto it = _refTrackers.find(refId);
        if (it != _refTrackers.end()) {
            it->second.count++;
        }
        else {
            _refTrackers.emplace(refId, std::to_string(std::stacktrace::current(2).at(0)));
        }
    }

    incRef();
}

void Object::decRef(u64 refId, bool dealloc) const noexcept
{
    if (_enableRefTracking) {
        std::lock_guard<std::mutex> lock(_refTrackerMutex);
        auto it = _refTrackers.find(refId);
        BEE_ASSERT(it != _refTrackers.end());
        if (--it->second.count == 0) {
            _refTrackers.erase(it);
        }
    }

    decRef(dealloc);
}

void Object::setEnableRefTracking(bool enable)
{
    _enableRefTracking = enable;
}

#endif // BEE_ENABLE_REF_TRACKING