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
    Logger::Instance().shutdown();
}

bool Application::initialize()
{
    if (m_initialized.load())
    {
        return true;
    }

    try
    {
        if (!Logger::Instance().isInitialized())
        {
            std::cerr << "[Bee] 日志初始化失败.\n";
        }
        
        BEE_INFO("{} 正在进行初始化.", m_name);

        PlatformManager::InitConfig config;
        config.enableWindow = true;
        
        _pPlatform = std::make_unique<PlatformManager>();
        if (!_pPlatform->initialize(config))
        {
            BEE_ERROR("平台管理器初始化失败.");
            return false;
        }

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

void Application::requestExit()
{
    m_running.store(false);
}
