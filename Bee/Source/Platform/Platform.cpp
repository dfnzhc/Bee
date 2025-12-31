/**
 * @File Platform.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/11
 * @Brief This file is part of Bee.
 */

#include "Platform.hpp"
#include "Utility/LogSink.hpp"
#include "Bee/Core/Concepts.hpp"

#include "SDLHeader.hpp"

using namespace bee;

namespace
{
SDL_MessageBoxFlags ConvertMessageBoxFlags(MessageBoxFlags flag)
{
    switch (flag)
    {
    case MessageBoxFlags::Info:
        return SDL_MESSAGEBOX_INFORMATION;
    case MessageBoxFlags::Warning:
        return SDL_MESSAGEBOX_WARNING;
    case MessageBoxFlags::Error:
        return SDL_MESSAGEBOX_ERROR;
    default: ;
    }

    return SDL_MESSAGEBOX_INFORMATION;
}

std::vector<SDL_MessageBoxButtonData> GatherMessageBoxButtons(MessageBoxButton type)
{
    // clang-format off
    constexpr SDL_MessageBoxButtonData btnOk{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, static_cast<int>(MessageBoxButton::Ok), "Ok"};
    constexpr SDL_MessageBoxButtonData btnCancel{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, static_cast<int>(MessageBoxButton::Cancel), "Cancel"};
    constexpr SDL_MessageBoxButtonData btnRetry{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, static_cast<int>(MessageBoxButton::Retry), "Retry"};
    constexpr SDL_MessageBoxButtonData btnYes{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, static_cast<int>(MessageBoxButton::Yes), "Yes"};
    constexpr SDL_MessageBoxButtonData btnNo{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, static_cast<int>(MessageBoxButton::No), "No"};
    constexpr SDL_MessageBoxButtonData btnAbort{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, static_cast<int>(MessageBoxButton::Abort), "Abort"};
    constexpr SDL_MessageBoxButtonData btnIgnore{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, static_cast<int>(MessageBoxButton::Ignore), "Ignore"};

    switch (type)
    {
    case MessageBoxButton::Ok:               return {btnOk};
    case MessageBoxButton::OkCancel:         return {btnOk, btnCancel};
    case MessageBoxButton::YesNo:            return {btnYes, btnNo};
    case MessageBoxButton::RetryCancel:      return {btnRetry, btnCancel};
    case MessageBoxButton::AbortRetryIgnore: return {btnAbort, btnRetry, btnIgnore};
    default: ;
    }
    // clang-format on
    return {};
}

SDL_Folder ConvertUserFolder(UserFolder folder)
{
    // clang-format off
    switch (folder)
    {
    case UserFolder::Home:        return SDL_FOLDER_HOME;
    case UserFolder::Desktop:     return SDL_FOLDER_DESKTOP;
    case UserFolder::Documents:   return SDL_FOLDER_DOCUMENTS;
    case UserFolder::Downloads:   return SDL_FOLDER_DOWNLOADS;
    case UserFolder::Music:       return SDL_FOLDER_MUSIC;
    case UserFolder::Pictures:    return SDL_FOLDER_PICTURES;
    case UserFolder::PublicShare: return SDL_FOLDER_PUBLICSHARE;
    case UserFolder::SavedGames:  return SDL_FOLDER_SAVEDGAMES;
    case UserFolder::Screenshots: return SDL_FOLDER_SCREENSHOTS;
    case UserFolder::Templates:   return SDL_FOLDER_TEMPLATES;
    case UserFolder::Videos:      return SDL_FOLDER_VIDEOS;
    }
    // clang-format on
    return SDL_FOLDER_HOME;
}

PathType ConvertPathType(SDL_PathType t)
{
    // clang-format off
    switch (t)
    {
    case SDL_PATHTYPE_NONE:      return PathType::None;
    case SDL_PATHTYPE_FILE:      return PathType::File;
    case SDL_PATHTYPE_DIRECTORY: return PathType::Directory;
    case SDL_PATHTYPE_OTHER:     return PathType::Other;
    }
    // clang-format on
    return PathType::Other;
}

} // namespace 

bool Platform::Initialize()
{
    // TODO: 日志
    Logger::Instance().addSink(std::make_unique<LogSink>());
    BEE_INFO("正在初始化平台设施...");

    u32 sdlFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
    if (!BEE_SDL_CALL(SDL_Init(sdlFlags)))
    {
        return false;
    }

    return true;
}

void Platform::Shutdown()
{
    SDL_Quit();
    Logger::Instance().clearSinks();
}

MessageBoxButton Platform::ShowMessageBox(std::string_view title, std::string_view msg, MessageBoxButton button, MessageBoxFlags flag)
{
    SDL_MessageBoxData data{};
    data.flags   = ConvertMessageBoxFlags(flag);
    data.window  = nullptr;
    data.title   = title.data();
    data.message = msg.data();

    const auto& buttons = GatherMessageBoxButtons(button);
    data.numbuttons     = static_cast<int>(buttons.size());
    data.buttons        = buttons.data();

    // 深色主题
    // clang-format off
    const SDL_MessageBoxColorScheme colorScheme = {
        {
            { 45,  45,  48 },   // Background
            { 240, 240, 240 },  // Text
            { 100, 100, 100 },  // Button Border
            { 62,  62,  66 },   // Button Background
            { 0,   122, 204 }   // Button Selected
        }
    };
    // clang-format on
    data.colorScheme = &colorScheme;

    int outButtonId = 0;
    if (!BEE_SDL_CALL(SDL_ShowMessageBox(&data, &outButtonId)))
    {
        return MessageBoxButton::None;
    }

    return static_cast<MessageBoxButton>(outButtonId);
}

bool Platform::SetClipboardText(std::string_view text)
{
    return BEE_SDL_CALL(SDL_SetClipboardText(text.data()));
}

bool Platform::SetPrimarySelectionText(std::string_view text)
{
    return BEE_SDL_CALL(SDL_SetPrimarySelectionText(text.data()));
}

std::string Platform::GetClipboardText()
{
    auto* p = BEE_SDL_CALL(SDL_GetClipboardText());
    if (!p)
        return {};

    std::string s{p};
    SDL_free(p);
    return s;
}

std::string Platform::GetPrimarySelectionText()
{
    auto* p = BEE_SDL_CALL(SDL_GetPrimarySelectionText());
    if (!p)
        return {};

    std::string s{p};
    SDL_free(p);
    return s;
}

bool Platform::HasClipboardText()
{
    return SDL_HasClipboardText();
}

bool Platform::HasPrimarySelectionText()
{
    return SDL_HasPrimarySelectionText();
}

u64 Platform::GetTicksMS()
{
    return SDL_GetTicks();
}

u64 Platform::GetTicksNS()
{
    return SDL_GetTicksNS();
}

u64 Platform::GetPerformanceCounter()
{
    return SDL_GetPerformanceCounter();
}

u64 Platform::GetPerformanceFrequency()
{
    return SDL_GetPerformanceFrequency();
}

void Platform::DelayMS(u32 ms)
{
    SDL_Delay(ms);
}

void Platform::DelayNS(u64 ns)
{
    SDL_DelayNS(ns);
}

void Platform::DelayPreciseNS(u64 ns)
{
    SDL_DelayPrecise(ns);
}

std::string Platform::GetBasePath()
{
    return BEE_SDL_CALL(SDL_GetBasePath());
}

std::string Platform::GetPrefPath(std::string_view org, std::string_view app)
{
    auto p = BEE_SDL_CALL(SDL_GetPrefPath(org.data(), app.data()));
    if (!p)
        return {};

    std::string s{p};
    SDL_free(p);
    return s;
}

std::string Platform::GetUserFolder(UserFolder folder)
{
    return BEE_SDL_CALL(SDL_GetUserFolder(ConvertUserFolder(folder)));
}

std::string Platform::GetCurrentDirectory()
{
    auto p = BEE_SDL_CALL(SDL_GetCurrentDirectory());
    if (!p)
        return {};

    std::string s{p};
    SDL_free(p);
    return std::move(s);
}

bool Platform::CreateDirectory(std::string_view path)
{
    if (path.empty())
        return false;

    return BEE_SDL_CALL(SDL_CreateDirectory(path.data()));
}

bool Platform::RemovePath(std::string_view path)
{
    if (path.empty())
        return false;

    return BEE_SDL_CALL(SDL_RemovePath(path.data()));
}

bool Platform::RenamePath(std::string_view oldPath, std::string_view newPath)
{
    if (oldPath.empty() || newPath.empty())
        return false;

    return BEE_SDL_CALL(SDL_RenamePath(oldPath.data(), newPath.data()));
}

bool Platform::CopyFile(std::string_view oldPath, std::string_view newPath)
{
    if (oldPath.empty() || newPath.empty())
        return false;

    return BEE_SDL_CALL(SDL_CopyFile(oldPath.data(), newPath.data()));
}

PathInfo Platform::GetPathInfo(std::string_view path)
{
    if (path.empty())
        return {};

    SDL_PathInfo info{};
    if (!BEE_SDL_CALL(SDL_GetPathInfo(path.data(), &info)))
        return {};

    PathInfo pi{};
    pi.type       = ConvertPathType(info.type);
    pi.size       = static_cast<u64>(info.size);
    pi.createTime = static_cast<u64>(info.create_time);
    pi.modifyTime = static_cast<u64>(info.modify_time);
    pi.accessTime = static_cast<u64>(info.access_time);

    return pi;
}
