/**
 * @File Object.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/22
 * @Brief This file is part of Bee.
 */

#pragma once

#include "./Defines.hpp"

#include <atomic>
#include <utility>
#include <cstdint>
#include <fmt/core.h>

#define BEE_ENABLE_REF_TRACKING 1

#if BEE_ENABLE_REF_TRACKING
#  include <map>
#  include <mutex>
#endif

namespace bee {

class BEE_API Object
{
public:
    Object() = default;

    Object(const Object&) { }

    Object& operator=(const Object&) { return *this; }

    Object(Object&&)            = delete;
    Object& operator=(Object&&) = delete;

    virtual ~Object() = default;

    virtual std::string_view className() const { return "Object"; }

    int refCount() const { return _refCount; };

    void incRef() const;

    void decRef(bool dealloc = true) const noexcept;

#if BEE_ENABLE_REF_TRACKING
    static void dumpAliveObjects();

    void dumpRefs() const;

    void incRef(u64 refId) const;
    void decRef(u64 refId, bool dealloc = true) const noexcept;

    void setEnableRefTracking(bool enable);
#endif

private:
    mutable std::atomic<uint32_t> _refCount{0};

#if BEE_ENABLE_REF_TRACKING
    struct RefTracker
    {
        uint32_t count;
        std::string origin;

        RefTracker(std::string origin_) : count(1), origin(std::move(origin_)) { }
    };

    mutable std::map<u64, RefTracker> _refTrackers;
    mutable std::mutex _refTrackerMutex;
    bool _enableRefTracking = true;
#endif
};

#if BEE_ENABLE_REF_TRACKING
static u64 NextRefId()
{
    static std::atomic<u64> sNextId = 0;
    return sNextId.fetch_add(1);
}
#endif

#define BEE_OBJECT(class_)                                                                                                                           \
public:                                                                                                                                              \
    std::string_view className() const override                                                                                                      \
    {                                                                                                                                                \
        return #class_;                                                                                                                              \
    }

template<typename T> class ref
{
public:
    static_assert(std::derived_from<T, Object>, "需要是 Object 的派生类型.");

    ref() = default;

    ref(std::nullptr_t) { }

    template<typename U = T>
        requires std::derived_from<U, Object> and std::convertible_to<U*, T*>
    ref(U* ptr) : _ptr(ptr)
    {
        if (_ptr)
            incRef((const Object*)(_ptr));
    }

    ref(const ref& r) : _ptr(r._ptr)
    {
        if (_ptr)
            incRef((const Object*)(_ptr));
    }

    template<typename U = T>
        requires std::derived_from<U, Object> and std::convertible_to<U*, T*>
    ref(const ref<U>& r) : _ptr(r._ptr)
    {
        if (_ptr)
            incRef((const Object*)(_ptr));
    }

    ref(ref&& r) noexcept
    : _ptr(r._ptr)
#if BEE_ENABLE_REF_TRACKING
      ,
      _refId(r._refId)
#endif
    {
        r._ptr = nullptr;
#if BEE_ENABLE_REF_TRACKING
        r._refId = u64(-1);
#endif
    }

    template<typename U>
        requires std::derived_from<U, Object> and std::convertible_to<U*, T*>
    ref(ref<U>&& r) noexcept
    : _ptr(r._ptr)
#if BEE_ENABLE_REF_TRACKING
      ,
      _refId(r._refId)
#endif
    {
        r._ptr = nullptr;
#if BEE_ENABLE_REF_TRACKING
        r._refId = u64(-1);
#endif
    }

    ~ref()
    {
        if (_ptr)
            decRef((const Object*)(_ptr));
    }

    ref& operator=(const ref& r) noexcept
    {
        if (r != *this) {
            if (r._ptr)
                incRef((const Object*)(r._ptr));
            T* prevPtr = _ptr;
            _ptr       = r._ptr;
            if (prevPtr)
                decRef((const Object*)(prevPtr));
        }
        return *this;
    }

    template<typename U>
        requires std::convertible_to<U*, T*>
    ref& operator=(const ref<U>& r) noexcept
    {
        if (r != *this) {
            if (r._ptr)
                incRef((const Object*)(r._ptr));
            T* prevPtr = _ptr;
            _ptr       = r._ptr;
            if (prevPtr)
                decRef((const Object*)(prevPtr));
        }
        return *this;
    }

    ref& operator=(ref&& r) noexcept
    {
        if (static_cast<void*>(&r) != this) {
            if (_ptr)
                decRef((const Object*)(_ptr));
            _ptr   = r._ptr;
            r._ptr = nullptr;
#if BEE_ENABLE_REF_TRACKING
            _refId   = r._refId;
            r._refId = u64(-1);
#endif
        }
        return *this;
    }

    template<typename U>
        requires std::convertible_to<U*, T*>
    ref& operator=(ref<U>&& r) noexcept
    {
        if (static_cast<void*>(&r) != this) {
            if (_ptr)
                decRef((const Object*)(_ptr));
            _ptr   = r._ptr;
            r._ptr = nullptr;
#if BEE_ENABLE_REF_TRACKING
            _refId   = r._refId;
            r._refId = u64(-1);
#endif
        }
        return *this;
    }

    template<typename U = T>
        requires std::convertible_to<U*, T*>
    void reset(U* ptr = nullptr) noexcept
    {
        if (ptr != _ptr) {
            if (ptr)
                incRef((const Object*)(ptr));
            T* prevPtr = _ptr;
            _ptr       = ptr;
            if (prevPtr)
                decRef((const Object*)(prevPtr));
        }
    }

    template<typename U = T>
        requires std::convertible_to<U*, T*> || std::convertible_to<T*, U*>
    bool operator==(const ref<U>& r) const
    {
        return _ptr == r._ptr;
    }

    template<typename U = T>
        requires std::convertible_to<U*, T*> || std::convertible_to<T*, U*>
    bool operator!=(const ref<U>& r) const
    {
        return _ptr != r._ptr;
    }

    template<typename U = T>
        requires std::convertible_to<U*, T*> || std::convertible_to<T*, U*>
    bool operator<(const ref<U>& r) const
    {
        return _ptr < r._ptr;
    }

    template<typename U = T>
        requires std::convertible_to<U*, T*>
    bool operator==(const U* ptr) const
    {
        return _ptr == ptr;
    }

    template<typename U = T>
        requires std::convertible_to<U*, T*>
    bool operator!=(const U* ptr) const
    {
        return _ptr != ptr;
    }

    // clang-format off

    bool operator==(std::nullptr_t) const { return _ptr == nullptr; }
    bool operator!=(std::nullptr_t) const { return _ptr != nullptr; }
    bool  operator<(std::nullptr_t) const { return _ptr < nullptr; }

    T* operator->() const { return _ptr; }
    T&  operator*() const { return *_ptr; }

    T* get() const { return _ptr; }
    operator bool() const { return _ptr != nullptr; }

    // clang-format on

    void swap(ref& r) noexcept
    {
        std::swap(_ptr, r._ptr);
#if BEE_ENABLE_REF_TRACKING
        std::swap(_refId, r._refId);
#endif
    }

private:
    inline void incRef(const Object* object)
    {
#if BEE_ENABLE_REF_TRACKING
        object->incRef(_refId);
#else
        object->incRef();
#endif
    }

    inline void decRef(const Object* object)
    {
#if BEE_ENABLE_REF_TRACKING
        object->decRef(_refId);
#else
        object->decRef();
#endif
    }

    T* _ptr = nullptr;

#if BEE_ENABLE_REF_TRACKING
    u64 _refId = NextRefId();
#endif

private:
    template<typename U> friend class ref;
};

template<class T, class... Args> ref<T> make_ref(Args&&... args)
{
    return ref<T>(new T(std::forward<Args>(args)...));
}

template<class T, class U> ref<T> static_ref_cast(const ref<U>& r) noexcept
{
    return ref<T>(static_cast<T*>(r.get()));
}

template<class T, class U> ref<T> dynamic_ref_cast(const ref<U>& r) noexcept
{
    return ref<T>(dynamic_cast<T*>(r.get()));
}

template<typename T> class BreakableReference
{
public:
    // clang-format off
    BreakableReference(const ref<T>& r) : mStrongRef(r), mWeakRef(mStrongRef.get()) { }
    BreakableReference(ref<T>&& r) : mStrongRef(r), mWeakRef(mStrongRef.get()) { }

    BreakableReference()                         = delete;
    BreakableReference& operator=(const ref<T>&) = delete;
    BreakableReference& operator=(ref<T>&&)      = delete;

    T* get() const { return mWeakRef; }

    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }

    operator ref<T>() const { return ref<T>(get()); }
    operator T*() const { return get(); }
    operator bool() const { return get() != nullptr; }

    void breakStrongReference() { mStrongRef.reset(); }

    // clang-format on

private:
    ref<T> mStrongRef;
    T* mWeakRef = nullptr;
};

} // namespace bee

template<typename T> struct fmt::formatter<bee::ref<T>> : formatter<const void*>
{
    template<typename FormatContext> auto format(const bee::ref<T>& ref, FormatContext& ctx) const
    {
        return formatter<const void*>::format(ref.get(), ctx);
    }
};

template<typename T> struct fmt::formatter<bee::BreakableReference<T>> : formatter<const void*>
{
    template<typename FormatContext> auto format(const bee::BreakableReference<T>& ref, FormatContext& ctx) const
    {
        return formatter<const void*>::format(ref.get(), ctx);
    }
};

namespace std {

template<typename T> void swap(::bee::ref<T>& x, ::bee::ref<T>& y) noexcept
{
    return x.swap(y);
}

template<typename T> struct hash<::bee::ref<T>>
{
    constexpr size_t operator()(const ::bee::ref<T>& r) const { return std::hash<T*>()(r.get()); }
};

} // namespace std