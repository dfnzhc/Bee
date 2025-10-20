/**
 * @File Guardian.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/20
 * @Brief This file is part of Bee.
 */

#include "Guardian.hpp"

using namespace Bee;

ScopeGuardian::ScopeGuardian() :
    _initialUncaughtExceptions(std::uncaught_exceptions())
{
}

ScopeGuardian::~ScopeGuardian() noexcept
{
    if (_dismissed)
    {
        return;
    }

    std::lock_guard lock(_mutex);

    bool hasExceptionNow = hasException();

    // 先处理异常回调
    if (hasExceptionNow && !_exceptionCallbacks.empty())
    {
        auto exceptionPtr = _manualException ? _manualException : std::current_exception();
        for (const auto& callback : _exceptionCallbacks)
        {
            try
            {
                callback(exceptionPtr);
            }
            catch (...)
            {
                // 异常回调中的异常只能忽略
            }
        }
    }

    // 然后按照注册的逆序执行清理回调
    for (auto it = _cleanupCallbacks.rbegin(); it != _cleanupCallbacks.rend(); ++it)
    {
        const auto& info = *it;
        if (info.shouldExecute(hasExceptionNow))
        {
            try
            {
                info.callback();
            }
            catch (const std::exception& e)
            {
                try
                {
                    const auto& sl = info.sourceLocation;
                    std::cerr << std::format("作用域护卫的清除回调调用时发生错误 '{}': {} ('{}' {}:{}).\n", info.description, e.what(), sl.function_name(), sl.file_name(), sl.line());
                }
                catch (...)
                {
                }
            }
        }
    }
}

ScopeGuardian& ScopeGuardian::onCleanup(CleanupCallback callback, CleanupTiming timing, const std::string& description,
                                        std::source_location sourceLocation)
{
    std::lock_guard lock(_mutex);
    _cleanupCallbacks.emplace_back(CallbackInfo{std::move(callback), description, std::move(sourceLocation), timing});
    return *this;
}

ScopeGuardian& ScopeGuardian::onException(ExceptionCallback callback)
{
    std::lock_guard lock(_mutex);
    _exceptionCallbacks.emplace_back(std::move(callback));
    return *this;
}

ScopeGuardian& ScopeGuardian::onSuccess(CleanupCallback callback, const std::string& description,
                                 std::source_location sourceLocation)
{
    return onCleanup(std::move(callback), CleanupTiming::OnSuccess, description, std::move(sourceLocation));
}

ScopeGuardian& ScopeGuardian::onFailure(CleanupCallback callback, const std::string& description,
                                 std::source_location sourceLocation)
{
    return onCleanup(std::move(callback), CleanupTiming::OnFailure, description, std::move(sourceLocation));
}

void ScopeGuardian::setException(const std::exception_ptr& ex)
{
    std::lock_guard lock(_mutex);
    _manualException = ex;
}

void ScopeGuardian::dismiss() noexcept
{
    std::lock_guard lock(_mutex);
    _dismissed = true;
}

bool ScopeGuardian::isDismissed() const noexcept
{
    std::lock_guard lock(_mutex);
    return _dismissed;
}

size_t ScopeGuardian::callbackCount() const noexcept
{
    std::lock_guard lock(_mutex);
    return _cleanupCallbacks.size();
}

bool ScopeGuardian::hasException() const noexcept
{
    return std::uncaught_exceptions() > _initialUncaughtExceptions || _manualException != nullptr;
}

bool ScopeGuardian::CallbackInfo::shouldExecute(bool hasException) const
{
    bool shouldExecute = false;
    switch (timing)
    {
        case CleanupTiming::Always:
            shouldExecute = true;
            break;
        case CleanupTiming::OnSuccess:
            shouldExecute = !hasException;
            break;
        case CleanupTiming::OnFailure:
        case CleanupTiming::OnException:
            shouldExecute = hasException;
            break;
    }

    return shouldExecute;
}
