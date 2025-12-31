/**
 * @File Platform.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/11
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Bee/Core/Defines.hpp"
#include "Bee/Core/Log.hpp"

namespace bee
{

// 消息框的类型
enum class MessageBoxFlags
{
    Info,
    Warning,
    Error
};

// 消息框按钮
enum class MessageBoxButton
{
    None   = 0x00,
    Ok     = 0x01,
    Cancel = 0x02,
    Retry  = 0x04,
    Yes    = 0x08,
    No     = 0x10,
    Abort  = 0x20,
    Ignore = 0x40,

    OkCancel         = Ok | Cancel,
    RetryCancel      = Retry | Cancel,
    YesNo            = Yes | No,
    AbortRetryIgnore = Abort | Retry | Ignore,
};

// 用户目录类型
enum class UserFolder
{
    Home,
    Desktop,
    Documents,
    Downloads,
    Music,
    Pictures,
    PublicShare,
    SavedGames,
    Screenshots,
    Templates,
    Videos,
};

enum class PathType
{
    None,
    File,
    Directory,
    Other,
};

struct PathInfo
{
    PathType type{};
    u64 size{};
    u64 createTime{};
    u64 modifyTime{};
    u64 accessTime{};
};

class Platform
{
public:
    static bool Initialize();
    static void Shutdown();

    // -------------------- 
    // 弹窗
    // clang-format off
    static MessageBoxButton ShowMessageBox(std::string_view title, std::string_view msg, MessageBoxButton button = MessageBoxButton::Ok, MessageBoxFlags flag = MessageBoxFlags::Info);
    // clang-format on
    // TODO: 选择、保存文件窗口

    // -------------------- 
    // 剪切板
    static bool SetClipboardText(std::string_view text);
    static bool SetPrimarySelectionText(std::string_view text);

    static std::string GetClipboardText();
    static std::string GetPrimarySelectionText();

    static bool HasClipboardText();
    static bool HasPrimarySelectionText();

    // -------------------- 
    // Timer
    static u64 GetTicksMS();
    static u64 GetTicksNS();
    static u64 GetPerformanceCounter();   // 当前计数
    static u64 GetPerformanceFrequency(); // 每秒计数

    static void DelayMS(u32 ms);
    static void DelayNS(u64 ns);
    static void DelayPreciseNS(u64 ns);

    // -------------------- 
    // 文件操作

    static std::string GetBasePath();
    static std::string GetPrefPath(std::string_view org, std::string_view app);
    static std::string GetUserFolder(UserFolder folder);
    static std::string GetCurrentDirectory();

    static bool CreateDirectory(std::string_view path);
    static bool RemovePath(std::string_view path);
    static bool RenamePath(std::string_view oldPath, std::string_view newPath);
    static bool CopyFile(std::string_view oldPath, std::string_view newPath);
    
    static PathInfo GetPathInfo(std::string_view path);

};

} // namespace bee
