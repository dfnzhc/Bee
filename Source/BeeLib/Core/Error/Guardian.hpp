/**
 * @File Guardian.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/20
 * @Brief This file is part of Bee.
 */

#pragma once

#include <expected>
#include "./Exception.hpp"

namespace Bee
{
    // ==================== 异常护卫 ====================

    template <typename F, class... Args>
    auto ExceptionGuardian(F callback, Args&&... args)
        -> std::expected<std::invoke_result_t<F, Args...>, std::exception_ptr>
    {
        static_assert(std::is_invocable_v<F, Args...>);
        try
        {
            if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>)
            {
                std::invoke(callback, std::forward<Args>(args)...);
                return {};
            }
            else
            {
                return std::invoke(callback, std::forward<Args>(args)...);
            }
        }
        catch (...)
        {
            return std::unexpected(std::current_exception());
        }
    }

    // ==================== 作用域守卫 ====================

    class ScopeGuardian
    {
    public:
        // 清理策略
        enum class CleanupTiming
        {
            Always,     // 总是执行清理
            OnSuccess,  // 仅在成功时执行
            OnFailure,  // 仅在失败时执行
            OnException // 仅在异常时执行
        };

        // 回调类型
        using CleanupCallback   = std::function<void()>;
        using ExceptionCallback = std::function<void(const std::exception_ptr&)>;

    public:
        // === 构造函数 ===
        ScopeGuardian();
        BEE_DISABLE_COPY(ScopeGuardian);

        // === 清理接口 ===
        ~ScopeGuardian() noexcept;
        ScopeGuardian& onCleanup(CleanupCallback callback, CleanupTiming timing = CleanupTiming::Always,
                                 const std::string& description                 = "unnamed",
                                 std::source_location sourceLocation = std::source_location::current());

        // === 设置回调 ===
        ScopeGuardian& onException(ExceptionCallback callback);
        ScopeGuardian& onSuccess(CleanupCallback callback, const std::string& description = "onSuccess",
                                 std::source_location sourceLocation = std::source_location::current());
        ScopeGuardian& onFailure(CleanupCallback callback, const std::string& description = "onFailure",
                                 std::source_location sourceLocation = std::source_location::current());

        // === 使用操作 ===

        // 手动设置异常状态
        void setException(const std::exception_ptr& ex);

        // 取消所有清理操作
        void dismiss() noexcept;

        // === 查询状态 ===
        bool isDismissed() const noexcept;
        size_t callbackCount() const noexcept;

    private:
        bool hasException() const noexcept;

    private:
        struct CallbackInfo
        {
            CleanupCallback callback;
            std::string description;
            std::source_location sourceLocation;
            CleanupTiming timing;

            bool shouldExecute(bool hasException) const;
        };

        std::vector<CallbackInfo> _cleanupCallbacks;
        std::vector<ExceptionCallback> _exceptionCallbacks;
        std::exception_ptr _manualException;
        mutable std::mutex _mutex;
        int _initialUncaughtExceptions;
        bool _dismissed = false;
    };
} // namespace Bee
