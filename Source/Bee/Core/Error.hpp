/**
 * @File Error.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/3/28
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Logger.hpp"
#include "Core/Portability.hpp"
#include "Core/Thirdparty.hpp"

#include <expected>
#include <iostream>

namespace bee {
// ==================
// Error results
// ==================

enum class Error
{
    Ok = 0,
    Failed, ///> General Failed.
};

inline constexpr std::string_view ErrorName(Error err)
{
    return me::enum_name(err);
}

template<typename T> using Result = std::expected<T, String>;

inline std::unexpected<String> Unexpected(StringView msg)
{
    return std::unexpected{String(msg)};
}

template<typename... Args> std::unexpected<String> Unexpected(std::format_string<Args...> fmt, Args&&... args)
{
    return std::unexpected{String(std::format(fmt, std::forward<Args>(args)...))};
}

// ==================
// Custom exception
// ==================

// clang-format off
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
[[noreturn]] BEE_API void ReportAssertion(const std::source_location& loc, StringView cond, StringView msg = {});

namespace details {
[[noreturn]] inline void ThrowException(const std::source_location& loc, StringView msg)
{
    ::bee::ThrowException(loc, msg);
}

template<typename... Args> [[noreturn]] inline void ThrowException(const std::source_location& loc, std::format_string<Args...> fmt, Args&&... args)
{
    ::bee::ThrowException(loc, std::format(fmt, std::forward<Args>(args)...));
}

[[noreturn]] inline void ReportAssertion(const std::source_location& loc, StringView cond)
{
    ::bee::ReportAssertion(loc, cond);
}

[[noreturn]] inline void ReportAssertion(const std::source_location& loc, StringView cond, StringView msg)
{
    ::bee::ReportAssertion(loc, cond, msg);
}

template<typename... Args> [[noreturn]] inline void ReportAssertion(const std::source_location& loc, StringView cond, std::format_string<Args...> fmt, Args&&... args)
{
    ::bee::ReportAssertion(loc, cond, std::format(fmt, std::forward<Args>(args)...));
}
} // namespace details

#define BEE_THROW(...) ::bee::details::ThrowException(std::source_location::current(), __VA_ARGS__)

#define BEE_CHECK(cond, ...)                                                                                                                                                                           \
    do {                                                                                                                                                                                               \
        if (!(cond))                                                                                                                                                                                   \
            BEE_THROW(__VA_ARGS__);                                                                                                                                                                    \
    } while (0)

#define BEE_UNIMPLEMENTED() BEE_THROW("Not Implemented!!")

#define BEE_UNREACHABLE() BEE_THROW("You shall not PASS!!!")

#define BEE_ASSERT(cond, ...)                                                                                                                                                                          \
    do {                                                                                                                                                                                               \
        if (!(cond)) {                                                                                                                                                                                 \
            ::bee::details::ReportAssertion(std::source_location::current(), #cond __VA_OPT__(, ) __VA_ARGS__);                                                                                        \
        }                                                                                                                                                                                              \
    } while (0)

#define BEE_ASSERT_OP(a, b, OP)                                                                                                                                                                        \
    do {                                                                                                                                                                                               \
        if (!((a)OP(b))) {                                                                                                                                                                             \
            ::bee::details::ReportAssertion(std::source_location::current(), std::format("{} {} {} ({} {} {})", #a, #OP, #b, a, #OP, b));                                                              \
        }                                                                                                                                                                                              \
    } while (0)

#define BEE_ASSERT_EQ(a, b) BEE_ASSERT_OP(a, b, ==)
#define BEE_ASSERT_NE(a, b) BEE_ASSERT_OP(a, b, !=)
#define BEE_ASSERT_GE(a, b) BEE_ASSERT_OP(a, b, >=)
#define BEE_ASSERT_GT(a, b) BEE_ASSERT_OP(a, b, >)
#define BEE_ASSERT_LE(a, b) BEE_ASSERT_OP(a, b, <=)
#define BEE_ASSERT_LT(a, b) BEE_ASSERT_OP(a, b, <)

#ifdef BEE_ENABLE_DEBUG

#  define BEE_DEBUG_ASSERT(cond, ...)   BEE_ASSERT(cond, __VA_ARGS__)
#  define BEE_DEBUG_ASSERT_OP(a, b, OP) BEE_ASSERT_OP(a, b, OP)

#  define BEE_DEBUG_ASSERT_EQ(a, b) BEE_DEBUG_ASSERT_OP(a, b, ==)
#  define BEE_DEBUG_ASSERT_NE(a, b) BEE_DEBUG_ASSERT_OP(a, b, !=)
#  define BEE_DEBUG_ASSERT_GE(a, b) BEE_DEBUG_ASSERT_OP(a, b, >=)
#  define BEE_DEBUG_ASSERT_GT(a, b) BEE_DEBUG_ASSERT_OP(a, b, >)
#  define BEE_DEBUG_ASSERT_LE(a, b) BEE_DEBUG_ASSERT_OP(a, b, <=)
#  define BEE_DEBUG_ASSERT_LT(a, b) BEE_DEBUG_ASSERT_OP(a, b, <)

#else // BEE_ENABLE_DEBUG

#  define BEE_DEBUG_ASSERT(cond, ...)                                                                                                                                                                  \
      do {                                                                                                                                                                                             \
          {                                                                                                                                                                                            \
          }                                                                                                                                                                                            \
      } while (0)
#  define BEE_DEBUG_ASSERT_OP(a, b, OP)                                                                                                                                                                \
      do {                                                                                                                                                                                             \
          {                                                                                                                                                                                            \
          }                                                                                                                                                                                            \
      } while (0)
#  define BEE_DEBUG_ASSERT_EQ(a, b) BEE_DEBUG_ASSERT_OP(a, b, ==)
#  define BEE_DEBUG_ASSERT_NE(a, b) BEE_DEBUG_ASSERT_OP(a, b, !=)
#  define BEE_DEBUG_ASSERT_GE(a, b) BEE_DEBUG_ASSERT_OP(a, b, >=)
#  define BEE_DEBUG_ASSERT_GT(a, b) BEE_DEBUG_ASSERT_OP(a, b, >)
#  define BEE_DEBUG_ASSERT_LE(a, b) BEE_DEBUG_ASSERT_OP(a, b, <=)
#  define BEE_DEBUG_ASSERT_LT(a, b) BEE_DEBUG_ASSERT_OP(a, b, <)

#endif // BEE_ENABLE_DEBUG

// ==================
// Error callback
// ==================

namespace details {
inline void CustomTerminateHandler()
{
    if (auto eptr = std::current_exception()) {
        std::rethrow_exception(eptr);
    }

    BEE_THROW("Exiting without exception.");
}

inline void SignalHandler(int signal)
{
    if (signal == SIGABRT)
        BEE_THROW("'SIGABRT' received!");

    BEE_THROW("Unexpected signal '{}' received!", signal);
}
} // namespace details

// ==================
// Guardian callback
// ==================

// TODO: How to handle the error? (Error handler, recovery...
template<typename F, class... Args> inline int Guardian(F callback, Args&&... args)
{
    std::set_terminate(details::CustomTerminateHandler);
    std::signal(SIGABRT, details::SignalHandler);

    int result = EXIT_FAILURE;
    try {
        result = std::invoke(callback, std::forward<Args>(args)...);
    } catch (const AssertionError& e) {
        std::cerr << "Assertion Failed: " << e.what();
        // LogFatal("Assertion Failed:\n{}", e.what()); // FIXME: logger not working
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what();
        // LogFatal("Exception:\n{}", e.what());
    } catch (...) {
        std::cerr << "Unknown Exception";
        // LogFatal("Unknown Exception");
    }

    return result;
}
} // namespace bee