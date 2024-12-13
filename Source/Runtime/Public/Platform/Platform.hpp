/**
 * @File Platform.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Utility/String.hpp"

#include "Platform/Components/MessageBox.hpp"
#include "Platform/Components/FileDialog.hpp"
#include "Platform/Components/Threads.hpp"

#include <filesystem>
#include <functional>
#include <optional>

namespace bee {

extern std::vector<StringView> SurfaceExtensions();

class BEE_API Platform
{
public:
    static int GetDisplayDpi();
    static float GetDisplayScaleFactor();

    static MessageBoxButton MakeMessageBox(const std::string& title,
                                           const std::string& msg,
                                           MessageBoxType type = MessageBoxType::Ok,
                                           MessageBoxIcon icon = MessageBoxIcon::None);

    static bool OpenFileDialog(const FileDialogFilterVec& filters, std::filesystem::path& path);
    static bool SaveFileDialog(const FileDialogFilterVec& filters, std::filesystem::path& path);
    static bool ChooseFolderDialog(std::filesystem::path& path);
    
    static void MonitorFileUpdates(const std::filesystem::path& path, const std::function<void()>& callback = {});
    static void CloseSharedFile(const std::filesystem::path& path);
    static time_t GetFileModifiedTime(const std::filesystem::path& path);

    static size_t ExecuteProcess(const std::string& appName, const std::string& commandLineArgs);
    static bool IsProcessRunning(size_t processID);
    static void TerminateProcess(size_t processID);

    static const std::filesystem::path& GetExecutablePath();
    static const std::filesystem::path& GetRuntimeDirectory();
    static const std::filesystem::path& GetAppDataDirectory();
    static const std::filesystem::path& GetHomeDirectory();

    static std::optional<std::string> GetEnvironmentVariable(const std::string& varName);

    static void SetKeyboardInterruptHandler(std::function<void()> handler);

    static bool IsDebuggerPresent();
    static void DebugBreak();
    static void PrintToDebugWindow(const std::string& s);

    static std::thread::native_handle_type GetCurrentThread();
    static void SetThreadAffinity(std::thread::native_handle_type thread, uint32_t affinityMask);
    static void SsetThreadPriority(std::thread::native_handle_type thread, ThreadPriorityType priority);

    static uint64_t GetTotalVirtualMemory();
    static uint64_t GetUsedVirtualMemory();
    static uint64_t GetProcessUsedVirtualMemory();

    static uint64_t GetCurrentRSS();
    static uint64_t GetPeakRSS();

    static uint32_t BitScanReverse(uint32_t a);
    static uint32_t BitScanForward(uint32_t a);
    static uint32_t Popcount(uint32_t a);

    // static SharedLibraryHandle LoadSharedLibrary(const std::filesystem::path& path);
    // static void ReleaseSharedLibrary(SharedLibraryHandle library);
    //
    // static void* GetProcAddress(SharedLibraryHandle library, const std::string& funcName);
};

} // namespace bee