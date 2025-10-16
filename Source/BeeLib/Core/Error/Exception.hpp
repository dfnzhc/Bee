/**
 * @File Exception.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/12
 * @Brief This file is part of Bee.
 */

#pragma once

#include <cassert>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <format>
#include <source_location>
#include <string>
#include <string_view>

#include "Core/Base/Defines.hpp"

namespace Bee
{
    // clang-format off
    class Exception : public std::exception
    {
    public:
        Exception() noexcept = default;
        Exception(const Exception& other) noexcept : exception(other) { _pWhat = other._pWhat; }
        explicit Exception(std::string_view what) : _pWhat(std::string(what)) { }
        ~Exception() override = default;

        Exception& operator=(const Exception&) = delete;
        BEE_NODISCARD const char* what() const noexcept override { return _pWhat.c_str(); }

    protected:
        std::string _pWhat;
    };

    class RuntimeError : public Exception
    {
    public:
        RuntimeError() noexcept = default;
        RuntimeError(const RuntimeError& other) noexcept : Exception(other) { _pWhat = other._pWhat; }
        explicit RuntimeError(std::string_view what) : Exception(what) { }

        ~RuntimeError() override = default;
    };

    class AssertionError : public Exception
    {
    public:
        AssertionError() noexcept = default;
        AssertionError(const AssertionError& other) noexcept : Exception(other) { _pWhat = other._pWhat; }
        explicit AssertionError(std::string_view what) : Exception(what) { }

        ~AssertionError() override = default;
    };

    BEE_NORETURN void ThrowException(const std::source_location& loc, std::string_view msg);
    BEE_NORETURN void ReportAssertion(const std::source_location& loc, std::string_view cond, std::string_view msg = {});

    namespace details {
        BEE_NORETURN inline void ThrowException(const std::source_location& loc, std::string_view msg)
        {
            ::Bee::ThrowException(loc, msg);
        }

        template <typename... Args>
        BEE_NORETURN void ThrowException(const std::source_location& loc, std::format_string<Args...> fmt, Args&&... args)
        {
            ::Bee::ThrowException(loc, std::format(fmt, std::forward<Args>(args)...));
        }

        BEE_NORETURN inline void ReportAssertion(const std::source_location& loc, std::string_view cond)
        {
            ::Bee::ReportAssertion(loc, cond);
        }

        BEE_NORETURN inline void ReportAssertion(const std::source_location& loc, std::string_view cond, std::string_view msg)
        {
            ::Bee::ReportAssertion(loc, cond, msg);
        }

        template <typename... Args>
        BEE_NORETURN void ReportAssertion(const std::source_location& loc, std::string_view cond, std::format_string<Args...> fmt, Args&&... args)
        {
            ::Bee::ReportAssertion(loc, cond, std::format(fmt, std::forward<Args>(args)...));
        }
    } // namespace details

    // clang-format on

    #define BEE_THROW(...) ::Bee::details::ThrowException(std::source_location::current(), __VA_ARGS__)

    #define BEE_CHECK(cond, ...)         \
        do {                             \
            if (!(cond)) [[unlikely]] {  \
                BEE_THROW(__VA_ARGS__);  \
            }                            \
        } while (0)


    #define BEE_ASSERT(cond, ...)                                                                                    \
        do {                                                                                                         \
            if (!(cond)) [[unlikely]] {                                                                              \
                ::Bee::details::ReportAssertion(std::source_location::current(), #cond __VA_OPT__(, ) __VA_ARGS__);  \
            }                                                                                                        \
        } while (0)

    #define BEE_ASSERT_OP(a, b, OP)                                                                                                            \
        do {                                                                                                                                   \
            if (!((a)OP(b))) [[unlikely]]  {                                                                                                   \
                ::Bee::details::ReportAssertion(std::source_location::current(), std::format("{} {} {} ({} {} {})", #a, #OP, #b, a, #OP, b));  \
            }                                                                                                                                  \
        } while (0)

    #define BEE_ASSERT_EQ(a, b) BEE_ASSERT_OP(a, b, ==)
    #define BEE_ASSERT_NE(a, b) BEE_ASSERT_OP(a, b, !=)
    #define BEE_ASSERT_GE(a, b) BEE_ASSERT_OP(a, b, >=)
    #define BEE_ASSERT_GT(a, b) BEE_ASSERT_OP(a, b, >)
    #define BEE_ASSERT_LE(a, b) BEE_ASSERT_OP(a, b, <=)
    #define BEE_ASSERT_LT(a, b) BEE_ASSERT_OP(a, b, <)

    #ifdef BEE_DEBUG

    #  define BEE_DEBUG_ASSERT(cond, ...)   BEE_ASSERT(cond, __VA_ARGS__)
    #  define BEE_DEBUG_ASSERT_OP(a, b, OP) BEE_ASSERT_OP(a, b, OP)

    #  define BEE_DEBUG_ASSERT_EQ(a, b) BEE_DEBUG_ASSERT_OP(a, b, ==)
    #  define BEE_DEBUG_ASSERT_NE(a, b) BEE_DEBUG_ASSERT_OP(a, b, !=)
    #  define BEE_DEBUG_ASSERT_GE(a, b) BEE_DEBUG_ASSERT_OP(a, b, >=)
    #  define BEE_DEBUG_ASSERT_GT(a, b) BEE_DEBUG_ASSERT_OP(a, b, >)
    #  define BEE_DEBUG_ASSERT_LE(a, b) BEE_DEBUG_ASSERT_OP(a, b, <=)
    #  define BEE_DEBUG_ASSERT_LT(a, b) BEE_DEBUG_ASSERT_OP(a, b, <)

    #else // BEE_DEBUG

    #  define BEE_DEBUG_ASSERT(cond, ...) do { { } } while (0)
    #  define BEE_DEBUG_ASSERT_OP(a, b, OP) do { { } } while (0)

    #  define BEE_DEBUG_ASSERT_EQ(a, b) BEE_DEBUG_ASSERT_OP(a, b, ==)
    #  define BEE_DEBUG_ASSERT_NE(a, b) BEE_DEBUG_ASSERT_OP(a, b, !=)
    #  define BEE_DEBUG_ASSERT_GE(a, b) BEE_DEBUG_ASSERT_OP(a, b, >=)
    #  define BEE_DEBUG_ASSERT_GT(a, b) BEE_DEBUG_ASSERT_OP(a, b, >)
    #  define BEE_DEBUG_ASSERT_LE(a, b) BEE_DEBUG_ASSERT_OP(a, b, <=)
    #  define BEE_DEBUG_ASSERT_LT(a, b) BEE_DEBUG_ASSERT_OP(a, b, <)

    #endif // BEE_DEBUG


    // ==================
    // Error callback
    // ==================
    namespace details
    {
        BEE_NORETURN inline void CustomTerminateHandler()
        {
            if (auto eptr = std::current_exception())
            {
                try
                {
                    std::rethrow_exception(eptr);
                }
                catch (const std::exception& e)
                {
                    // Or use a more robust logging mechanism
                    std::cerr << "Terminate called with an exception: " << e.what() << std::endl;
                }
                catch (...)
                {
                    std::cerr << "Terminate called with an unknown exception." << std::endl;
                }
            }
            else
            {
                std::cerr << "Terminate called without an active exception." << std::endl;
            }
            std::abort();
        }

        BEE_NORETURN inline void SignalHandler(int signal)
        {
            const char* msg;
            if (signal == SIGABRT)
            {
                msg = "Fatal Error: 'SIGABRT' received! Terminating.\n";
            }
            else
            {
                msg = "Fatal Error: Unexpected signal received! Terminating.\n";
            }

            std::cerr << msg;
            std::_Exit(EXIT_FAILURE);
        }
    } // namespace details

    // ==================
    // Guardian callback
    // ==================
    template <typename F, class... Args>
    inline bool Guardian(F callback, Args&&... args)
    {
        static_assert(std::is_invocable_v<F, Args...>);
        std::set_terminate(details::CustomTerminateHandler);
        std::signal(SIGABRT, details::SignalHandler);

        bool bResult = false;
        // clang-format off
        try {
            if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>) {
                std::invoke(callback, std::forward<Args>(args)...);
                bResult = true;
            }
            else {
                bResult = static_cast<bool>(std::invoke(callback, std::forward<Args>(args)...));
            }
        } catch (const AssertionError& e) {
            std::cerr << "Assertion Failed: " << e.what() << std::endl;
        } catch (const std::exception& e) { std::cerr << "Exception: " << e.what() << std::endl; } catch (...) {
            std::cerr << "Unknown Exception" << std::endl;
        }
        // clang-format on

        return bResult;
    }
} // namespace Bee
