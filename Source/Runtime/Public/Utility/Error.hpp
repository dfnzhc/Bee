/**
 * @File Error.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/21
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Portability.hpp"
#include "Utility/Enum.hpp"
#include "Utility/Logger.hpp"
#include "Utility/String.hpp"

namespace bee {

enum class Error
{
    Ok = 0,
    Failed,          // 通常意义的失败
    InitializeError, // 初始化失败
    VulkanError,     // Vulkan 调用失败
    CheckError,      // 当不满足预定条件
};

inline constexpr std::string_view ErrorName(Error err)
{
    return me::enum_name(err);
}

#define BEE_REPORT_IF_FAILED(expr)                                                                                                                   \
    do {                                                                                                                                             \
        auto err = expr;                                                                                                                             \
        if (err != Error::Ok) [[unlikely]] {                                                                                                         \
            LogError("调用失败：'{}' = {}", #expr, ErrorName(err));                                                                                  \
            return err;                                                                                                                              \
        }                                                                                                                                            \
        else                                                                                                                                         \
            ((void)0);                                                                                                                               \
    } while (false)

// clang-format off
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4'275) // allow dllexport on classes dervied from STL
#endif

class BEE_API Exception : public std::exception
{
public:
    Exception() noexcept = default;
    Exception(const Exception& other) noexcept : exception(other) { _pWhat = other._pWhat; }
    explicit Exception(StringView what) : _pWhat(std::make_shared<String>(what)) { }
    ~Exception() override = default;

    Exception& operator=(const Exception&) = delete;
    BEE_NODISCARD const char* what() const noexcept override { return _pWhat ? _pWhat->c_str() : ""; }

protected:
    std::shared_ptr<String> _pWhat;
};

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

class BEE_API RuntimeError : public Exception
{
public:
    RuntimeError() noexcept = default;
    RuntimeError(const RuntimeError& other) noexcept : Exception(other) { _pWhat = other._pWhat; }
    explicit RuntimeError(StringView what) : Exception(what) { }

    ~RuntimeError() override = default;
};

class BEE_API AssertionError : public Exception
{
public:
    AssertionError() noexcept = default;
    AssertionError(const AssertionError& other) noexcept : Exception(other) { _pWhat = other._pWhat; }
    explicit AssertionError(StringView what) : Exception(what) { }

    ~AssertionError() override = default;
};

// clang-format on

[[noreturn]] BEE_API void ThrowException(const std::source_location& loc, StringView msg);
[[noreturn]] BEE_API void ReportAssertion(const std::source_location& loc, std::string_view cond, std::string_view msg = {});

namespace internal {

[[noreturn]] inline void ThrowException(const std::source_location& loc, StringView msg)
{
    ::bee::ThrowException(loc, msg);
}

template<typename... Args> [[noreturn]] inline void ThrowException(const std::source_location& loc, fmt::format_string<Args...> fmt, Args&&... args)
{
    ::bee::ThrowException(loc, fmt::format(fmt, std::forward<Args>(args)...));
}

[[noreturn]] inline void ReportAssertion(const std::source_location& loc, std::string_view cond)
{
    ::bee::ReportAssertion(loc, cond);
}

[[noreturn]] inline void ReportAssertion(const std::source_location& loc, std::string_view cond, std::string_view msg)
{
    ::bee::ReportAssertion(loc, cond, msg);
}

template<typename... Args>
[[noreturn]] inline void ReportAssertion(const std::source_location& loc, std::string_view cond, std::format_string<Args...> fmt, Args&&... args)
{
    ::bee::ReportAssertion(loc, cond, std::format(fmt, std::forward<Args>(args)...));
}

} // namespace internal

#define BEE_THROW(...) ::bee::internal::ThrowException(std::source_location::current(), __VA_ARGS__)

#define BEE_CHECK(cond, ...)                                                                                                                         \
    do {                                                                                                                                             \
        if (!(cond))                                                                                                                                 \
            BEE_THROW(__VA_ARGS__);                                                                                                                  \
    } while (0)

#define BEE_UNIMPLEMENTED() BEE_THROW("还未实现的部分")

#define BEE_UNREACHABLE() BEE_THROW("You shall not PASS!!!")

#define BEE_ASSERT(cond, ...)                                                                                                                        \
    do {                                                                                                                                             \
        if (!(cond)) {                                                                                                                               \
            ::bee::internal::ReportAssertion(std::source_location::current(), #cond __VA_OPT__(,) __VA_ARGS__);                                                   \
        }                                                                                                                                            \
    } while (0)

#define BEE_ASSERT_OP(a, b, OP)                                                                                                                      \
    do {                                                                                                                                             \
        if (!(a OP b)) {                                                                                                                             \
            ::bee::internal::ReportAssertion(std::source_location::current(), std::format("{} {} {} ({} {} {})", #a, #OP, #b, a, #OP, b));           \
        }                                                                                                                                            \
    } while (0)

#define BEE_ASSERT_EQ(a, b) BEE_ASSERT_OP(a, b, ==)
#define BEE_ASSERT_NE(a, b) BEE_ASSERT_OP(a, b, !=)
#define BEE_ASSERT_GE(a, b) BEE_ASSERT_OP(a, b, >=)
#define BEE_ASSERT_GT(a, b) BEE_ASSERT_OP(a, b, >)
#define BEE_ASSERT_LE(a, b) BEE_ASSERT_OP(a, b, <=)
#define BEE_ASSERT_LT(a, b) BEE_ASSERT_OP(a, b, <)

#ifdef BEE_ENABLE_DEBUG_ASSERTS

#  define BEE_DEBUG_ASSERT(cond, ...)   BEE_ASSERT(cond, __VA_ARGS__)
#  define BEE_DEBUG_ASSERT_OP(a, b, OP) BEE_ASSERT_OP(a, b, OP)

#  define BEE_DEBUG_ASSERT_EQ(a, b) BEE_DEBUG_ASSERT_OP(a, b, ==)
#  define BEE_DEBUG_ASSERT_NE(a, b) BEE_DEBUG_ASSERT_OP(a, b, !=)
#  define BEE_DEBUG_ASSERT_GE(a, b) BEE_DEBUG_ASSERT_OP(a, b, >=)
#  define BEE_DEBUG_ASSERT_GT(a, b) BEE_DEBUG_ASSERT_OP(a, b, >)
#  define BEE_DEBUG_ASSERT_LE(a, b) BEE_DEBUG_ASSERT_OP(a, b, <=)
#  define BEE_DEBUG_ASSERT_LT(a, b) BEE_DEBUG_ASSERT_OP(a, b, <)

#else // BEE_ENABLE_DEBUG_ASSERTS

#  define BEE_DEBUG_ASSERT(cond, ...)                                                                                                                \
      do {                                                                                                                                           \
          {                                                                                                                                          \
          }                                                                                                                                          \
      } while (0)
#  define BEE_DEBUG_ASSERT_OP(a, b, OP)                                                                                                              \
      do {                                                                                                                                           \
          {                                                                                                                                          \
          }                                                                                                                                          \
      } while (0)
#  define BEE_DEBUG_ASSERT_EQ(a, b) BEE_DEBUG_ASSERT_OP(a, b, ==)
#  define BEE_DEBUG_ASSERT_NE(a, b) BEE_DEBUG_ASSERT_OP(a, b, !=)
#  define BEE_DEBUG_ASSERT_GE(a, b) BEE_DEBUG_ASSERT_OP(a, b, >=)
#  define BEE_DEBUG_ASSERT_GT(a, b) BEE_DEBUG_ASSERT_OP(a, b, >)
#  define BEE_DEBUG_ASSERT_LE(a, b) BEE_DEBUG_ASSERT_OP(a, b, <=)
#  define BEE_DEBUG_ASSERT_LT(a, b) BEE_DEBUG_ASSERT_OP(a, b, <)

#endif // BEE_ENABLE_ASSERTS

/// 异常处理
// TODO: 目前只做报告，考虑支持恢复？
template<typename CallbackT, typename ResultT = int> inline int CatchAndReportAllExceptions(CallbackT callback, ResultT errorResult = 1)
{
    ResultT result = errorResult;
    try {
        result = callback();
    } catch (const AssertionError& e) {
        fmt::println("断言错误:\n\n{}", e.what());
    } catch (const std::exception& e) {
        LogFatal("发生异常:\n\n{}", e.what());
    } catch (...) {
        LogFatal("未知异常发生");
    }
    return result;
}

} // namespace bee
