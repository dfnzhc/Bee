/**
 * @File Engine.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/1
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"
#include "Core/Memory/Memory.hpp"
#include "Core/Utility/ClassTypes.hpp"

namespace bee {
class BEE_API Engine final : public CMoveable
{
public:
    Engine();
    ~Engine() override;

    /**
     * @brief Initializes the engine.
     * @return Returns true on success, false on failure.
     */
    bool initialize();

    /**
     * @brief Shuts down the engine, releasing all resources.
     */
    void shutdown();

private:


};
} // namespace bee