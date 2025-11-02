/**
 * @File Application.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/13
 * @Brief This file is part of Bee.
 */


#include "Application.hpp"

#include "PlatformManager.hpp"

using namespace Bee;

Application::Application(std::string name) :
    m_name(std::move(name))
{
}

Application::~Application()
{
    if (m_initialized.load())
    {
        shutdown();
    }
}

bool Application::initialize()
{
    if (m_initialized.load())
    {
        return true;
    }

    try
    {
        BEE_INFO("{} 正在进行初始化.", m_name);

        PlatformManager::InitConfig config;
        config.enableWindow = true;

        _pPlatform = std::make_unique<PlatformManager>();
        if (auto res = _pPlatform->initialize(config); !res)
        {
            BEE_ERROR("平台管理器初始化失败, 错误码: '{:x}'.", res.error().code());
            return false;
        }

        // clang-format off
        _pPlatform->setCommonEventCallback([this](const CommonEvent& event) { handleCommonEvent(event); });
        _pPlatform->setWindowEventCallback([this](const WindowEvent& event) { handleWindowEvent(event); });
        _pPlatform->setDropEventCallback([this](const DropEvent& event) { handleDropEvent(event); });
        
        _pPlatform->setKeyEventCallback([this](const KeyboardEvent& event) { handleKeyboardEvent(event); });
        _pPlatform->setMouseButtonCallback([this](const MouseButtonEvent& event) { handleMouseButtonEvent(event); });
        _pPlatform->setMouseMotionCallback([this](const MouseMotionEvent& event) { handleMouseMotionEvent(event); });
        _pPlatform->setMouseWheelCallback([this](const MouseWheelEvent& event) { handleMouseWheelEvent(event); });
        // clang-format on

        if (!onInitialize())
        {
            BEE_ERROR("应用程序初始化失败.");
            return false;
        }

        BEE_INFO("{} 初始化完毕.", m_name);
        m_initialized.store(true);

        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Bee] 应用程序类型初始化失败, 发生了异常: " << e.what() << std::endl;
        return false;
    }
}

void Application::shutdown()
{
    if (!m_initialized.load())
    {
        return;
    }

    try
    {
        m_running.store(false);
        onShutdown();
        m_initialized.store(false);

        BEE_INFO("{} 已关闭.", m_name);
        Logger::Instance().shutdown();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception during shutdown: " << e.what() << std::endl;
    }
}

int Application::run()
{
    if (!m_initialized.load())
    {
        BEE_ERROR("应用程序未初始化.");
        return EXIT_FAILURE;
    }

    if (m_running.load())
    {
        BEE_ERROR("应用程序已经在运行了.");
        return EXIT_FAILURE;
    }

    try
    {
        if (!onPrepareRun())
        {
            BEE_ERROR("运行前准备失败.");
            return EXIT_FAILURE;
        }

        m_running.store(true);
        while (m_running.load())
        {
            update();
            drawFrame();
        }

        onFinishRun();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Bee] 运行时发生了异常: " << e.what() << std::endl;
        m_running.store(false);
        return EXIT_FAILURE;
    }
}

bool Application::isRunning() const
{
    return m_running.load();
}

bool Application::onInitialize()
{
    return true;
}

void Application::onShutdown()
{
}

bool Application::onPrepareRun()
{
    return true;
}

void Application::onFinishRun()
{
}

void Application::drawFrame()
{
}

void Application::onHandleKeyboardEvent(const KeyboardEvent&)
{
}

void Application::onHandleMouseButtonEvent(const MouseButtonEvent&)
{
}

void Application::onHandleMouseMotionEvent(const MouseMotionEvent&)
{
}

void Application::onHandleMouseWheelEvent(const MouseWheelEvent&)
{
}

void Application::onHandleCommonEvent(const CommonEvent&)
{
}

void Application::onHandleWindowEvent(const WindowEvent&)
{
}

void Application::onHandleDropEvent(const DropEvent&)
{
}

void Application::requestExit()
{
    m_running.store(false);
}

void Application::update() const
{
    _pPlatform->update();
}

void Application::handleKeyboardEvent(const KeyboardEvent& event)
{
    if (event.keyCode == KeyCode::Escape)
    {
        if (event.isPressed)
            requestExit();
    }

    onHandleKeyboardEvent(event);
}

void Application::handleCommonEvent(const CommonEvent& event)
{
    if (event.type == PlatformEventType::Quit)
    {
        requestExit();
    }

    onHandleCommonEvent(event);
}

void Application::handleWindowEvent(const WindowEvent& event)
{
    onHandleWindowEvent(event);
}

void Application::handleDropEvent(const DropEvent& event)
{
    onHandleDropEvent(event);
}

void Application::handleMouseButtonEvent(const MouseButtonEvent& event)
{
    onHandleMouseButtonEvent(event);
}

void Application::handleMouseMotionEvent(const MouseMotionEvent& event)
{
    onHandleMouseMotionEvent(event);
}

void Application::handleMouseWheelEvent(const MouseWheelEvent& event)
{
    onHandleMouseWheelEvent(event);
}
