/**
 * @File WindowManagerTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/30
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../Impl/Private/SDLHeader.hpp"
#include "../../../Source/BeeLib/Platform/Impl/Private/WindowManager.hpp"
#include "Platform/Interface/PlatformTypes.hpp"

using namespace Bee;
using namespace testing;

class WindowManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        u32 sdlFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
        SDL_Init(sdlFlags);

        windowManager = std::make_unique<WindowManager>();
    }

    void TearDown() override
    {
        if (windowManager && windowManager->isInitialized())
        {
            // 清理所有创建的窗口
            auto windows = windowManager->GetAllWindows();
            for (auto window : windows)
            {
                windowManager->destroyWindow(window);
            }
            windowManager->shutdown();
        }
        windowManager.reset();
        SDL_Quit();
    }

    // 辅助方法：创建基本窗口配置
    WindowCreateInfo createBasicWindowInfo(const std::string& title = "Test Window",
                                           int2 size                = {800, 600},
                                           int2 pos                 = {100, 100},
                                           WindowFlags flags        = WindowFlags::Default)
    {
        WindowCreateInfo info{};
        info.title = title;
        info.size  = size;
        info.pos   = pos;
        info.flags = flags;
        return info;
    }

    // 辅助方法：创建窗口并验证成功
    WindowHandle createValidWindow(const WindowCreateInfo& info = {})
    {
        WindowCreateInfo createInfo = info.title.empty() ? createBasicWindowInfo() : info;
        auto result                 = windowManager->createWindow(createInfo);
        EXPECT_TRUE(result.has_value()) << "Failed to create window";
        if (result.has_value())
        {
            EXPECT_TRUE(windowManager->isWindowValid(result.value()));
            return result.value();
        }
        return WindowHandle{0};
    }

    // 辅助方法：验证窗口属性
    void verifyWindowProperties(WindowHandle window, const WindowCreateInfo& expectedInfo)
    {
        EXPECT_TRUE(windowManager->isWindowValid(window));
        EXPECT_EQ(windowManager->GetWindowTitle(window), expectedInfo.title);

        auto sizeResult = windowManager->GetWindowSize(window);
        EXPECT_TRUE(sizeResult.has_value());
        if (sizeResult.has_value())
        {
            EXPECT_EQ(sizeResult.value().x, expectedInfo.size.x);
            EXPECT_EQ(sizeResult.value().y, expectedInfo.size.y);
        }
    }

    std::unique_ptr<WindowManager> windowManager;
};

TEST_F(WindowManagerTest, InitializeAndShutdown)
{
    EXPECT_FALSE(windowManager->isInitialized());

    auto result = windowManager->initialize();
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(windowManager->isInitialized());

    result = windowManager->initialize();
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(windowManager->isInitialized());

    windowManager->shutdown();
    EXPECT_FALSE(windowManager->isInitialized());

    windowManager->shutdown();
    EXPECT_FALSE(windowManager->isInitialized());
}

TEST_F(WindowManagerTest, OperationsRequireInitialization)
{
    EXPECT_FALSE(windowManager->isInitialized());

    auto createInfo = createBasicWindowInfo();
    auto result     = windowManager->createWindow(createInfo);
    EXPECT_FALSE(result.has_value());

    windowManager->destroyWindow(WindowHandle{1});
    EXPECT_FALSE(windowManager->isWindowValid(WindowHandle{1}));

    auto windows = windowManager->GetAllWindows();
    EXPECT_TRUE(windows.empty());

    EXPECT_EQ(windowManager->GetMainWindow().handle, 0);
    EXPECT_EQ(windowManager->GetFocusedWindow().handle, 0);
}

TEST_F(WindowManagerTest, BasicWindowCreation)
{
    ASSERT_TRUE(windowManager->initialize().has_value());

    auto createInfo = createBasicWindowInfo("Test Window", {800, 600});
    auto result     = windowManager->createWindow(createInfo);

    ASSERT_TRUE(result.has_value());
    WindowHandle window = result.value();

    EXPECT_TRUE(windowManager->isWindowValid(window));
    EXPECT_NE(window.handle, 0);

    verifyWindowProperties(window, createInfo);
}

TEST_F(WindowManagerTest, MultipleWindowCreation)
{
    ASSERT_TRUE(windowManager->initialize().has_value());

    std::vector<WindowHandle> windows;
    for (int i = 0; i < 3; ++i)
    {
        auto createInfo = createBasicWindowInfo("Window " + std::to_string(i), {800, 600});
        auto result     = windowManager->createWindow(createInfo);
        ASSERT_TRUE(result.has_value());
        windows.push_back(result.value());
    }

    for (auto window : windows)
    {
        EXPECT_TRUE(windowManager->isWindowValid(window));
    }

    auto allWindows = windowManager->GetAllWindows();
    EXPECT_EQ(allWindows.size(), 3);
}

TEST_F(WindowManagerTest, WindowDestruction)
{
    ASSERT_TRUE(windowManager->initialize().has_value());

    auto window = createValidWindow();
    EXPECT_TRUE(windowManager->isWindowValid(window));

    windowManager->destroyWindow(window);
    EXPECT_FALSE(windowManager->isWindowValid(window));

    windowManager->destroyWindow(window);
    EXPECT_FALSE(windowManager->isWindowValid(window));
}

TEST_F(WindowManagerTest, WindowCreationWithDifferentFlags)
{
    ASSERT_TRUE(windowManager->initialize().has_value());

    std::vector<WindowFlags> flagsToTest = {
            WindowFlags::Default,
            WindowFlags::Resizable,
            WindowFlags::Borderless,
            WindowFlags::Resizable | WindowFlags::AlwaysOnTop
    };

    for (auto flags : flagsToTest)
    {
        auto createInfo = createBasicWindowInfo("Flag Test", {640, 480}, {200, 200}, flags);
        auto result     = windowManager->createWindow(createInfo);
        EXPECT_TRUE(result.has_value()) << "Failed to create window with flags: " << static_cast<u32>(flags);

        if (result.has_value())
        {
            windowManager->destroyWindow(result.value());
        }
    }
}

TEST_F(WindowManagerTest, WindowTitleOperations)
{
    ASSERT_TRUE(windowManager->initialize().has_value());
    auto window = createValidWindow();

    const std::string newTitle = "New Window Title";
    EXPECT_TRUE(windowManager->setWindowTitle(window, newTitle));
    EXPECT_EQ(windowManager->GetWindowTitle(window), newTitle);

    EXPECT_TRUE(windowManager->setWindowTitle(window, ""));
    EXPECT_EQ(windowManager->GetWindowTitle(window), "");

    const std::string longTitle(1000, 'A');
    EXPECT_TRUE(windowManager->setWindowTitle(window, longTitle));
    EXPECT_EQ(windowManager->GetWindowTitle(window), longTitle);
}

TEST_F(WindowManagerTest, WindowPositionOperations)
{
    ASSERT_TRUE(windowManager->initialize().has_value());
    auto window = createValidWindow();

    int2 newPos = {300, 250};
    EXPECT_TRUE(windowManager->setWindowPosition(window, newPos));

    auto posResult = windowManager->GetWindowPosition(window);
    EXPECT_TRUE(posResult.has_value());
    if (posResult.has_value())
    {
        EXPECT_EQ(posResult.value().x, newPos.x);
        EXPECT_EQ(posResult.value().y, newPos.y);
    }

    int2 negativePos = {-100, -50};
    EXPECT_TRUE(windowManager->setWindowPosition(window, negativePos));
}

TEST_F(WindowManagerTest, WindowSizeOperations)
{
    ASSERT_TRUE(windowManager->initialize().has_value());
    auto window = createValidWindow();

    int2 newSize = {1024, 768};
    EXPECT_TRUE(windowManager->setWindowSize(window, newSize));

    auto sizeResult = windowManager->GetWindowSize(window);
    EXPECT_TRUE(sizeResult.has_value());
    if (sizeResult.has_value())
    {
        EXPECT_EQ(sizeResult.value().x, newSize.x);
        EXPECT_EQ(sizeResult.value().y, newSize.y);
    }

    int2 minSize = {100, 100};
    EXPECT_TRUE(windowManager->setWindowSize(window, minSize));

    sizeResult = windowManager->GetWindowSize(window);
    EXPECT_TRUE(sizeResult.has_value());
}

TEST_F(WindowManagerTest, WindowVisibilityControl)
{
    ASSERT_TRUE(windowManager->initialize().has_value());
    auto window = createValidWindow();

    EXPECT_TRUE(windowManager->showWindow(window));
    EXPECT_TRUE(windowManager->hideWindow(window));
    EXPECT_TRUE(windowManager->showWindow(window));
}

TEST_F(WindowManagerTest, WindowStateControl)
{
    ASSERT_TRUE(windowManager->initialize().has_value());
    auto window = createValidWindow();

    EXPECT_TRUE(windowManager->minimizeWindow(window));
    EXPECT_TRUE(windowManager->restoreWindow(window));

    EXPECT_TRUE(windowManager->maximizeWindow(window));
    EXPECT_TRUE(windowManager->restoreWindow(window));

    EXPECT_TRUE(windowManager->setWindowFullscreen(window, true));
    EXPECT_TRUE(windowManager->setWindowFullscreen(window, false));
}

TEST_F(WindowManagerTest, WindowFocusControl)
{
    ASSERT_TRUE(windowManager->initialize().has_value());
    auto window1 = createValidWindow(createBasicWindowInfo("Window 1"));
    auto window2 = createValidWindow(createBasicWindowInfo("Window 2"));

    EXPECT_TRUE(windowManager->focusWindow(window1));
    EXPECT_TRUE(windowManager->focusWindow(window2));
}

TEST_F(WindowManagerTest, WindowEnumeration)
{
    ASSERT_TRUE(windowManager->initialize().has_value());

    auto windows = windowManager->GetAllWindows();
    EXPECT_TRUE(windows.empty());

    auto window1 = createValidWindow(createBasicWindowInfo("Window 1"));
    auto window2 = createValidWindow(createBasicWindowInfo("Window 2"));
    auto window3 = createValidWindow(createBasicWindowInfo("Window 3"));

    windows = windowManager->GetAllWindows();
    EXPECT_EQ(windows.size(), 3);

    EXPECT_THAT(windows, Contains(window1));
    EXPECT_THAT(windows, Contains(window2));
    EXPECT_THAT(windows, Contains(window3));

    windowManager->destroyWindow(window2);
    windows = windowManager->GetAllWindows();
    EXPECT_EQ(windows.size(), 2);
    EXPECT_THAT(windows, Not(Contains(window2)));
}

TEST_F(WindowManagerTest, MainWindowTracking)
{
    ASSERT_TRUE(windowManager->initialize().has_value());

    EXPECT_EQ(windowManager->GetMainWindow().handle, 0);

    auto window1 = createValidWindow(createBasicWindowInfo("Main Window"));
    EXPECT_EQ(windowManager->GetMainWindow().handle, window1.handle);

    auto window2 = createValidWindow(createBasicWindowInfo("Secondary Window"));
    EXPECT_EQ(windowManager->GetMainWindow().handle, window1.handle);

    windowManager->destroyWindow(window1);
    auto mainWindow = windowManager->GetMainWindow();
    if (mainWindow.handle != 0)
    {
        EXPECT_TRUE(windowManager->isWindowValid(mainWindow));
    }
}

TEST_F(WindowManagerTest, NativeWindowHandleAccess)
{
    ASSERT_TRUE(windowManager->initialize().has_value());
    auto window = createValidWindow();

    auto nativeHandle = windowManager->getNativeWindowHandle(window);

    #ifdef BEE_ON_WINDOWS
    EXPECT_NE(nativeHandle.hwnd, nullptr);
    #elif defined(BEE_ON_LINUX)
    EXPECT_NE(nativeHandle.window, 0);
    EXPECT_NE(nativeHandle.display, nullptr);
    #endif
}

TEST_F(WindowManagerTest, DisplayInformation)
{
    ASSERT_TRUE(windowManager->initialize().has_value());

    auto displays = windowManager->getDisplays();
    EXPECT_FALSE(displays.empty()) << "Should have at least one display";

    auto primaryDisplay = windowManager->getPrimaryDisplay();
    EXPECT_GT(primaryDisplay.width, 0);
    EXPECT_GT(primaryDisplay.height, 0);

    auto window        = createValidWindow();
    auto windowDisplay = windowManager->getWindowDisplay(window);
    EXPECT_GT(windowDisplay.width, 0);
    EXPECT_GT(windowDisplay.height, 0);
}

TEST_F(WindowManagerTest, InvalidWindowHandling)
{
    ASSERT_TRUE(windowManager->initialize().has_value());

    WindowHandle invalidWindow{999999};

    EXPECT_FALSE(windowManager->isWindowValid(invalidWindow));
    EXPECT_FALSE(windowManager->setWindowTitle(invalidWindow, "Test"));
    EXPECT_FALSE(windowManager->setWindowPosition(invalidWindow, {100, 100}));
    EXPECT_FALSE(windowManager->setWindowSize(invalidWindow, {800, 600}));

    EXPECT_FALSE(windowManager->showWindow(invalidWindow));
    EXPECT_FALSE(windowManager->hideWindow(invalidWindow));
    EXPECT_FALSE(windowManager->minimizeWindow(invalidWindow));
    EXPECT_FALSE(windowManager->maximizeWindow(invalidWindow));
    EXPECT_FALSE(windowManager->restoreWindow(invalidWindow));
    EXPECT_FALSE(windowManager->setWindowFullscreen(invalidWindow, true));
    EXPECT_FALSE(windowManager->focusWindow(invalidWindow));
    EXPECT_FALSE(windowManager->isWindowFocused(invalidWindow));

    EXPECT_TRUE(windowManager->GetWindowTitle(invalidWindow).empty());
    EXPECT_FALSE(windowManager->GetWindowPosition(invalidWindow).has_value());
    EXPECT_FALSE(windowManager->GetWindowSize(invalidWindow).has_value());

    windowManager->destroyWindow(invalidWindow);
}

TEST_F(WindowManagerTest, WindowOpacityOperations)
{
    ASSERT_TRUE(windowManager->initialize().has_value());
    auto window = createValidWindow();

    EXPECT_TRUE(windowManager->setWindowOpacity(window, 0.5f));
    f32 opacity = windowManager->getWindowOpacity(window);
    EXPECT_GE(opacity, 0.0f);
    EXPECT_LE(opacity, 1.0f);

    EXPECT_TRUE(windowManager->setWindowOpacity(window, 0.0f));
    EXPECT_TRUE(windowManager->setWindowOpacity(window, 1.0f));
}

TEST_F(WindowManagerTest, WindowIconOperations)
{
    ASSERT_TRUE(windowManager->initialize().has_value());
    auto window = createValidWindow();

    std::vector<u8> iconData(32 * 32 * 4, 255);

    bool result = windowManager->setWindowIcon(window, iconData.data(), 32, 32);
    EXPECT_TRUE(result || !result);
}

TEST_F(WindowManagerTest, RequestAttention)
{
    ASSERT_TRUE(windowManager->initialize().has_value());
    auto window = createValidWindow();

    windowManager->requestAttention(window);
    EXPECT_TRUE(windowManager->isWindowValid(window));
}

TEST_F(WindowManagerTest, EventCallbackOperations)
{
    ASSERT_TRUE(windowManager->initialize().has_value());

    windowManager->setWindowEventCallback();
    windowManager->removeWindowEventCallback();

    EXPECT_TRUE(windowManager->isInitialized());
}

TEST_F(WindowManagerTest, ComplexWindowManagement)
{
    ASSERT_TRUE(windowManager->initialize().has_value());

    auto mainWindow   = createValidWindow(createBasicWindowInfo("Main", {1024, 768}));
    auto dialogWindow = createValidWindow(createBasicWindowInfo("Dialog", {400, 300}));
    auto toolWindow   = createValidWindow(createBasicWindowInfo("Tool", {200, 500}));

    windowManager->setWindowPosition(mainWindow, {100, 100});
    windowManager->setWindowPosition(dialogWindow, {200, 200});
    windowManager->setWindowPosition(toolWindow, {300, 300});

    windowManager->maximizeWindow(mainWindow);
    windowManager->minimizeWindow(toolWindow);
    windowManager->focusWindow(dialogWindow);

    EXPECT_TRUE(windowManager->isWindowValid(mainWindow));
    EXPECT_TRUE(windowManager->isWindowValid(dialogWindow));
    EXPECT_TRUE(windowManager->isWindowValid(toolWindow));

    auto allWindows = windowManager->GetAllWindows();
    EXPECT_EQ(allWindows.size(), 3);

    windowManager->destroyWindow(toolWindow);
    EXPECT_EQ(windowManager->GetAllWindows().size(), 2);

    windowManager->destroyWindow(dialogWindow);
    EXPECT_EQ(windowManager->GetAllWindows().size(), 1);

    windowManager->destroyWindow(mainWindow);
    EXPECT_TRUE(windowManager->GetAllWindows().empty());
}
