/**
 * @File Property.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/13
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Defines.hpp"
#include "Base/Thirdparty.hpp"

namespace bee {

#define Property_EngineRunningName "EngineRunning"

class BEE_API Property final
{
public:
    
    /// ==========================
    /// Property Functions
    /// ==========================

    enum class Type
    {
        Unknown = 0, Boolean, Float32, SInt64, String
    };

    // clang-format off
    static bool Has(StringView propName);
    static bool Lock(StringView propName);
    static void Unlock(StringView propName);
    
    static Type GetType(StringView propName);
    static bool CheckType(StringView propName, Type type);

    static bool SetBool(StringView propName,    bool value);
    static bool SetFloat(StringView propName,   f32 value);
    static bool SetNumber(StringView propName,  i64 value);
    static bool SetString(StringView propName,  StringView value);

    static bool GetBool(StringView propName, bool defaultValue = false);
    static f32  GetFloat(StringView propName, f32 defaultValue = 0);
    static i64  GetNumber(StringView propName, i64 defaultValue = 0);
    static StringView GetString(StringView propName, StringView defaultValue = "");
    // clang-format on

    /// ==========================
    /// Engine Functions
    /// ==========================
    static bool IsEngineRunning();
    static void RequestEngineExit();
};
} // namespace bee