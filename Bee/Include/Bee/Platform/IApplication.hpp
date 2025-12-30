/**
 * @File IApplication.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/30
 * @Brief This file is part of Bee.
 */

#pragma once

#include <memory>

namespace bee
{
class IApplication
{
public:
    virtual ~IApplication() = default;

    virtual bool initialize(int argc, char* argv[]) = 0;
    virtual void runFrame() = 0;

    virtual void processEvent() = 0;

    virtual void shutdown() = 0;
    virtual void requestQuit() = 0;
    virtual bool shouldQuit() const = 0;
};

extern std::unique_ptr<IApplication> CreateApplication();

#define BEE_CREATE_APP(appClassName)                \
std::unique_ptr<IApplication> CreateApplication()   \
{                                                   \
    return std::make_unique<appClassName>();        \
}

} // namespace bee
