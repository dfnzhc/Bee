/**
 * @File VK_DeviceDriver.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include "GFX/GFX_DeviceDriver.hpp"

namespace bee {

class VK_Context;

class BEE_API VK_DeviceDriver final : public GFX_DeviceDriver
{
public:
    VK_DeviceDriver(VK_Context* context);
    
    ~VK_DeviceDriver() override;
    Error create(u32 deviceIndex, u32 frameCount) override;
    void destroy() override;
    
    // -------------------------
    // 组件创建
    // -------------------------
    
    void createBuffer()      override;
    void createTexture()     override;
    void createSampler()     override;
    void createSemaphore()   override;
    void createFence()       override;
    void createSwapChain()   override;
    void createFramebuffer() override;
    void createRenderPass()  override;
    void createShader()      override;

    void createComputePipeline() override;
    void createRenderPipeline()  override;
    bool createPipelineCache()   override;

    void createInputLayout() override;
    void createUniformSet()  override;
    void createQueryPool()   override;

    void createCommandQueue()  override;
    void createCommandPool()   override;
    void createCommandBuffer() override;

    // -------------------------
    // 组件操作
    // -------------------------

    /// ***** 缓冲区 *****
    u8* mapBuffer()    override;
    void unmapBuffer() override;

    /// ***** 纹理 *****
    u8* mapTexture()    override;
    void unmapTexture() override;
    Error waitFence()   override;

    /// ***** 交换链 *****
    Error resizeSwapChain()                override;
    void acquireFramebufferFromSwapChain() override;
    void renderPassInSwapChain()           override;
    void swapChainFormat()                 override;

    /// ***** 着色器 *****
    Vector<u8> compileShaderToBinary() override; // SPIRV 反射

    /// ***** 时间戳 *****
    void queryResults()               override;
    u64 queryResultToTime(u64 result) override;

    /// ***** 命令池 *****
    void commandQueueFamily()             override;
    Error commandQueueExecuteAndPresent() override;

    /// ***** 命令缓冲区 *****
    bool commandBufferBegin()            override;
    void commandBufferEnd()              override;
    bool commandBufferBeginSecondary()   override;
    void commandBufferExecuteSecondary() override;

    // -------------------------
    // 命令操作
    // -------------------------

    /// ***** 资源操作 *****
    void clearBuffer() override;
    void copyBuffer()  override;

    void copyTexture()    override;
    void resolveTexture() override;
    void clearTexture()   override;

    void copyBufferToTexture() override;
    void copyTextureToBuffer() override;
    void bindPushConstants()   override;

    /// ***** 渲染 *****
    void beginRenderPass()   override;
    void endRenderPass()     override;
    void nextRenderSubpass() override;
    void setViewport()       override;
    void setScissor()        override;
    void clearAttachments()  override;

    void bindRenderPipeline()   override;
    void bindRenderUniformSet() override;

    void draw()                     override;
    void drawIndexed()              override;
    void drawIndexedIndirect()      override;
    void drawIndexedIndirectCount() override;
    void drawIndirect()             override;
    void drawIndirectCount()        override;

    void bindVertexBuffers() override;
    void bindIndexBuffer()   override;

    void setBlendConstants() override;
    void setLineWidth()      override;

    void bindComputePipeline()   override;
    void bindComputeUniformSet() override;

    void dispatch()         override;
    void dispatchIndirect() override;
    void queryPoolReset()   override;
    void writeTimestamp()   override;

    void beginLabel() override;
    void endLabel()   override;

    void setObjectName()                        override;
    BEE_NODISCARD u64 getResourceNativeHandle() override;
    BEE_NODISCARD u64 getTotalMemoryUsed()      override;
    BEE_NODISCARD u64 getLimit()                override;
    BEE_NODISCARD bool hasFeature()             override;
    BEE_NODISCARD String apiName() const        override;
    BEE_NODISCARD String apiVersion() const     override;

    BEE_NODISCARD bool isCompositeAlphaSupported() const override;
    
private:
    VK_Context* _context = nullptr;
};

} // namespace bee