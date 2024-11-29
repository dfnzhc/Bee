/**
 * @File RenderDriver.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/21
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Core/Defines.hpp>
#include <Core/Portability.hpp>
#include <Utility/Error.hpp>

namespace bee {

// TODO: 当前只是列出接口

using Handle    = int;
using BufferID  = int;
using TextureID = int;

template<typename T> using Vector = std::vector<T>;

class BEE_API RenderDriver
{
public:
    virtual ~RenderDriver() = default;
    
    virtual Error create(u32 deviceIndex, u32 frameCount) = 0;
    virtual void destroy() = 0;

    // -------------------------
    // 组件创建
    // -------------------------

    virtual void createBuffer()      = 0;
    virtual void createTexture()     = 0;
    virtual void createSampler()     = 0;
    virtual void createSemaphore()   = 0;
    virtual void createFence()       = 0;
    virtual void createSwapChain()   = 0;
    virtual void createFramebuffer() = 0;
    virtual void createRenderPass()  = 0;
    virtual void createShader()      = 0;

    virtual void createComputePipeline() = 0;
    virtual void createRenderPipeline()  = 0;
    virtual bool createPipelineCache()   = 0;

    virtual void createInputLayout() = 0;
    virtual void createUniformSet()  = 0;
    virtual void createQueryPool()   = 0;

    virtual void createCommandQueue()  = 0;
    virtual void createCommandPool()   = 0;
    virtual void createCommandBuffer() = 0;

    // -------------------------
    // 组件操作
    // -------------------------

    /// ***** 缓冲区 *****
    virtual u8* mapBuffer()    = 0;
    virtual void unmapBuffer() = 0;

    /// ***** 纹理 *****
    virtual u8* mapTexture()    = 0;
    virtual void unmapTexture() = 0;
    virtual Error waitFence()   = 0;

    /// ***** 交换链 *****
    virtual Error resizeSwapChain()                = 0;
    virtual void acquireFramebufferFromSwapChain() = 0;
    virtual void renderPassInSwapChain()           = 0;
    virtual void swapChainFormat()                 = 0;

    /// ***** 着色器 *****
    virtual Vector<u8> compileShaderToBinary() = 0; // SPIRV 反射

    /// ***** 时间戳 *****
    virtual void queryResults()               = 0;
    virtual u64 queryResultToTime(u64 result) = 0;

    /// ***** 命令池 *****
    virtual void commandQueueFamily()             = 0;
    virtual Error commandQueueExecuteAndPresent() = 0;

    /// ***** 命令缓冲区 *****
    virtual bool commandBufferBegin()            = 0;
    virtual void commandBufferEnd()              = 0;
    virtual bool commandBufferBeginSecondary()   = 0;
    virtual void commandBufferExecuteSecondary() = 0;

    // -------------------------
    // 命令操作
    // -------------------------

    /// ***** 资源操作 *****
    virtual void clearBuffer() = 0;
    virtual void copyBuffer()  = 0;

    virtual void copyTexture()    = 0;
    virtual void resolveTexture() = 0;
    virtual void clearTexture()   = 0;

    virtual void copyBufferToTexture() = 0;
    virtual void copyTextureToBuffer() = 0;
    virtual void bindPushConstants()   = 0;

    /// ***** 渲染 *****
    virtual void beginRenderPass()   = 0;
    virtual void endRenderPass()     = 0;
    virtual void nextRenderSubpass() = 0;
    virtual void setViewport()       = 0;
    virtual void setScissor()        = 0;
    virtual void clearAttachments()  = 0;

    virtual void bindRenderPipeline()   = 0;
    virtual void bindRenderUniformSet() = 0;

    virtual void draw()                     = 0;
    virtual void drawIndexed()              = 0;
    virtual void drawIndexedIndirect()      = 0;
    virtual void drawIndexedIndirectCount() = 0;
    virtual void drawIndirect()             = 0;
    virtual void drawIndirectCount()        = 0;

    virtual void bindVertexBuffers() = 0;
    virtual void bindIndexBuffer()   = 0;

    virtual void setBlendConstants() = 0;
    virtual void setLineWidth()      = 0;

    virtual void bindComputePipeline()   = 0;
    virtual void bindComputeUniformSet() = 0;

    virtual void dispatch()         = 0;
    virtual void dispatchIndirect() = 0;
    virtual void queryPoolReset()   = 0;
    virtual void writeTimestamp()   = 0;

    virtual void beginLabel() = 0;
    virtual void endLabel()   = 0;

    virtual void setObjectName()                        = 0;
    BEE_NODISCARD virtual u64 getResourceNativeHandle() = 0;
    BEE_NODISCARD virtual u64 getTotalMemoryUsed()      = 0;
    BEE_NODISCARD virtual u64 getLimit()                = 0;
    BEE_NODISCARD virtual bool hasFeature()             = 0;
    BEE_NODISCARD virtual String apiName() const        = 0;
    BEE_NODISCARD virtual String apiVersion() const     = 0;

    BEE_NODISCARD virtual bool isCompositeAlphaSupported() const { return false; }
};

} // namespace bee