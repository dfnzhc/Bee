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

#ifdef BEE_ENABLE_DEBUG
#  define BEE_ENABLE_REF_TRACKING 1
#endif

#if BEE_ENABLE_REF_TRACKING
#  include <map>
#  include <mutex>
#endif

#if BEE_ENABLE_REF_TRACKING
#  define BEE_REF_CONST
#else
#  define BEE_REF_CONST const
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

    u32 refCount() const { return _refCount; }

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
    mutable std::atomic<u32> _refCount{0};

#if BEE_ENABLE_REF_TRACKING
    struct RefTracker
    {
        u32 count;
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

#define BEE_OBJECT(class_)                                                                                                                                                                             \
public:                                                                                                                                                                                                \
    std::string_view className() const override                                                                                                                                                        \
    {                                                                                                                                                                                                  \
        return #class_;                                                                                                                                                                                \
    }

template<typename T> class Ref
{
public:
    static_assert(std::derived_from<T, Object>, "Must derived from class [Object].");

    Ref() = default;

    Ref(std::nullptr_t) { }

    template<typename U = T>
        requires std::derived_from<U, Object> and std::convertible_to<U*, T*>
    Ref(U* ptr) : _ptr(ptr)
    {
        if (_ptr)
            incRef(reinterpret_cast<const Object*>(_ptr));
    }

    Ref(const Ref& r) : _ptr(r._ptr)
    {
        if (_ptr)
            incRef(reinterpret_cast<const Object*>(_ptr));
    }

    template<typename U = T>
        requires std::derived_from<U, Object> and std::convertible_to<U*, T*>
    Ref(const Ref<U>& r) : _ptr(r._ptr)
    {
        if (_ptr)
            incRef(reinterpret_cast<const Object*>(_ptr));
    }

    Ref(Ref&& r) noexcept
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
    Ref(Ref<U>&& r) noexcept
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

    ~Ref()
    {
        if (_ptr)
            decRef((const Object*)(_ptr));
    }

    Ref& operator=(const Ref& r) noexcept
    {
        if (r != *this) {
            if (r._ptr)
                incRef(reinterpret_cast<const Object*>(r._ptr));
            T* prevPtr = _ptr;
            _ptr       = r._ptr;
            if (prevPtr)
                decRef(reinterpret_cast<const Object*>(prevPtr));
        }
        return *this;
    }

    template<typename U>
        requires std::convertible_to<U*, T*>
    Ref& operator=(const Ref<U>& r) noexcept
    {
        if (r != *this) {
            if (r._ptr)
                incRef(reinterpret_cast<const Object*>(r._ptr));
            T* prevPtr = _ptr;
            _ptr       = r._ptr;
            if (prevPtr)
                decRef(reinterpret_cast<const Object*>(prevPtr));
        }
        return *this;
    }

    Ref& operator=(Ref&& r) noexcept
    {
        if (static_cast<void*>(&r) != this) {
            if (_ptr)
                decRef(reinterpret_cast<const Object*>(_ptr));
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
    Ref& operator=(Ref<U>&& r) noexcept
    {
        if (static_cast<void*>(&r) != this) {
            if (_ptr)
                decRef(reinterpret_cast<const Object*>(_ptr));
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
                incRef(reinterpret_cast<const Object*>(ptr));
            T* prevPtr = _ptr;
            _ptr       = ptr;
            if (prevPtr)
                decRef(reinterpret_cast<const Object*>(prevPtr));
        }
    }

    template<typename U = T>
        requires std::convertible_to<U*, T*> || std::convertible_to<T*, U*>
    bool operator==(const Ref<U>& r) const
    {
        return _ptr == r._ptr;
    }

    template<typename U = T>
        requires std::convertible_to<U*, T*> || std::convertible_to<T*, U*>
    bool operator!=(const Ref<U>& r) const
    {
        return _ptr != r._ptr;
    }

    template<typename U = T>
        requires std::convertible_to<U*, T*> || std::convertible_to<T*, U*>
    bool operator<(const Ref<U>& r) const
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

    void swap(Ref& r) noexcept
    {
        std::swap(_ptr, r._ptr);
#if BEE_ENABLE_REF_TRACKING
        std::swap(_refId, r._refId);
#endif
    }

private:
    inline void incRef(const Object* object) BEE_REF_CONST
    {
#if BEE_ENABLE_REF_TRACKING
        object->incRef(_refId);
#else
        object->incRef();
#endif
    }

    inline void decRef(const Object* object) BEE_REF_CONST
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
    template<typename U> friend class Ref;
};

template<class T, class... Args> Ref<T> MakeRef(Args&&... args)
{
    return Ref<T>(new T(std::forward<Args>(args)...));
}

template<class T, class U> Ref<T> StaticRefCast(const Ref<U>& r) noexcept
{
    return Ref<T>(static_cast<T*>(r.get()));
}

template<class T, class U> Ref<T> DynamicRefCast(const Ref<U>& r) noexcept
{
    return Ref<T>(dynamic_cast<T*>(r.get()));
}

template<typename T> class WeakRef
{
public:
    // clang-format off
    WeakRef(const Ref<T>& r) : _ref(r), _weakRef(_ref.get()) { }
    WeakRef(Ref<T>&& r) : _ref(r), _weakRef(_ref.get()) { }

    WeakRef()                         = delete;
    WeakRef& operator=(const Ref<T>&) = delete;
    WeakRef& operator=(Ref<T>&&)      = delete;

    T* get() const { return _weakRef; }

    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }

    operator Ref<T>() const { return Ref<T>(get()); }
    operator T*() const { return get(); }
    operator bool() const { return get() != nullptr; }

    void breakRef() { _ref.reset(); }

    // clang-format on

private:
    Ref<T> _ref;
    T* _weakRef = nullptr;
};
} // namespace bee

template<typename T> struct fmt::formatter<bee::Ref<T>> : formatter<const void*>
{
    template<typename FormatContext> auto format(const bee::Ref<T>& ref, FormatContext& ctx) const { return formatter<const void*>::format(ref.get(), ctx); }
};

template<typename T> struct fmt::formatter<bee::WeakRef<T>> : formatter<const void*>
{
    template<typename FormatContext> auto format(const bee::WeakRef<T>& ref, FormatContext& ctx) const { return formatter<const void*>::format(ref.get(), ctx); }
};

namespace std {
template<typename T> void swap(::bee::Ref<T>& x, ::bee::Ref<T>& y) noexcept
{
    return x.swap(y);
}

template<typename T> struct hash<::bee::Ref<T>>
{
    constexpr size_t operator()(const ::bee::Ref<T>& r) const { return std::hash<T*>()(r.get()); }
};
} // namespace std