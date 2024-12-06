/**
 * @File MessageBox.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"

namespace bee {

enum class MessageBoxIcon
{
    None,
    Info,
    Warning,
    Error
};

enum class MessageBoxType
{
    Ok,
    OkCancel,
    RetryCancel,
    AbortRetryIgnore,
    YesNo,
};

enum class MessageBoxButton
{
    Ok,
    Retry,
    Cancel,
    Abort,
    Ignore,
    Yes,
    No,
};

} // namespace bee