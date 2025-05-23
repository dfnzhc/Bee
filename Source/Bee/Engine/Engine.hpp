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

struct EngineConfig
{
    void* windowHandle = nullptr;
};

class BEE_API Engine final : public CMoveable
{
public:
    Engine();
    ~Engine() override;

    /**
     * @brief Pre initializes the engine, basically build the common subsystems.
     * @return Returns true on success, false on failure.
     */
    bool preInitialize();
    
    /**
     * @brief Initializes the engine.
     * @return Returns true on success, false on failure.
     */
    bool initialize(EngineConfig config);

    /**
     * @brief Shuts down the engine, releasing all resources.
     */
    void shutdown();

    /**
     * @brief Handle the input events.
     * @param event Key or Mouse event.
     */
    void onInputEvent(Ptr<class InputEventBase> event) const;

private:
    EngineConfig _config;

};
} // namespace bee