#include "Tensor/Core/Storage.hpp"
#include "Base/Memory/Allocator.hpp"
#include "Base/Diagnostics/Error.hpp"

#if defined(BEE_TENSOR_WITH_CUDA)
#    include <cuda_runtime.h>
#endif

namespace bee
{

namespace
{

    // 最小 pinned host allocator：实现 IAllocator 接口，转发到 cudaMallocHost / cudaFreeHost。
    class PinnedHostAllocator final : public IAllocator
    {
    public:
        static auto instance() noexcept -> PinnedHostAllocator&
        {
            static PinnedHostAllocator inst;
            return inst;
        }

        [[nodiscard]] auto allocate(std::size_t nbytes, std::size_t /*alignment*/) -> Result<void*> override
        {
#if defined(BEE_TENSOR_WITH_CUDA)
            if (nbytes == 0)
                return static_cast<void*>(nullptr);
            void* ptr = nullptr;
            const auto err = cudaMallocHost(&ptr, nbytes);
            if (err != cudaSuccess) {
                return std::unexpected(make_error(
                    "cudaMallocHost failed", Severity::Recoverable, static_cast<int>(err)
                ));
            }
            return ptr;
#else
            (void)nbytes;
            return std::unexpected(make_error("CUDA not available for pinned host memory", Severity::Recoverable));
#endif
        }

        auto deallocate(void* p, std::size_t /*nbytes*/, std::size_t /*alignment*/) noexcept -> void override
        {
#if defined(BEE_TENSOR_WITH_CUDA)
            if (p)
                (void)cudaFreeHost(p);
#else
            (void)p;
#endif
        }

        [[nodiscard]] auto device() const noexcept -> Device override
        {
            return Device::CPU; // pinned host memory 虽然是 CPU 内存，但可被 CUDA 快速访问
        }

    private:
        PinnedHostAllocator() = default;
    };

} // namespace

Storage::Storage(void* data, std::size_t nbytes, std::size_t alignment, Device device, MemoryKind memory_kind, IAllocator* allocator) noexcept
    : data_(data)
    , nbytes_(nbytes)
    , alignment_(alignment)
    , device_(device)
    , memory_kind_(memory_kind)
    , allocator_(allocator)
{
}

Storage::~Storage()
{
    if (data_ != nullptr)
        allocator_->deallocate(data_, nbytes_, alignment_);
}

auto Storage::allocate(std::size_t nbytes, IAllocator& allocator) -> Result<std::shared_ptr<Storage>>
{
    auto result = allocator.allocate(nbytes, 64u);
    if (!result)
        return std::unexpected(std::move(result.error()));

    // 根据 device 推断 memory_kind
    MemoryKind kind = MemoryKind::Host;
    if (allocator.device() == Device::CUDA)
    {
        kind = MemoryKind::Device;
    }

    // Storage 构造函数为私有，无法使用 make_shared，直接用 new
    return std::shared_ptr<Storage>(new Storage(result.value(), nbytes, 64u, allocator.device(), kind, &allocator));
}

auto Storage::allocate(std::size_t nbytes, IAllocator& allocator, MemoryKind memory_kind) -> Result<std::shared_ptr<Storage>>
{
    // Workspace 属于 runtime-owned 语义，生命周期不由 Storage 析构管控。
    // 调用方应直接使用 tensor::cuda::request_workspace() 获取 workspace 指针，
    // 不可通过 Storage::allocate 创建 Workspace 类型 Storage。
    if (memory_kind == MemoryKind::Workspace)
        return std::unexpected(make_error(
            "Storage::allocate: MemoryKind::Workspace 不允许通过 Storage 创建，"
            "请使用 tensor::cuda::request_workspace()",
            Severity::Recoverable
        ));

    // HostPinned 路径使用专用 pinned host allocator
    IAllocator* actual_allocator = &allocator;
    if (memory_kind == MemoryKind::HostPinned) {
        actual_allocator = &PinnedHostAllocator::instance();
    }

    auto result = actual_allocator->allocate(nbytes, 64u);
    if (!result)
        return std::unexpected(std::move(result.error()));

    // 使用显式传入的 memory_kind（不再推断）
    return std::shared_ptr<Storage>(new Storage(result.value(), nbytes, 64u, actual_allocator->device(), memory_kind, actual_allocator));
}

auto Storage::data() noexcept -> void*
{
    return data_;
}

auto Storage::data() const noexcept -> const void*
{
    return data_;
}

auto Storage::nbytes() const noexcept -> std::size_t
{
    return nbytes_;
}

auto Storage::device() const noexcept -> Device
{
    return device_;
}

auto Storage::allocator() const noexcept -> IAllocator&
{
    return *allocator_;
}

auto Storage::memory_kind() const noexcept -> MemoryKind
{
    return memory_kind_;
}

auto Storage::is_pinned() const noexcept -> bool
{
    return memory_kind_ == MemoryKind::HostPinned;
}

} // namespace bee
