#include "Tensor/Core/Tensor.hpp"
#include "Base/Memory/Allocator.hpp"
#include "Base/Diagnostics/Check.hpp"
#include "Tensor/Core/Storage.hpp"
#include "Tensor/Cuda/CudaAllocator.hpp"
#include "Tensor/Cuda/Backend.hpp"
#include "Tensor/Cuda/ExecContext.hpp"
#include "Tensor/Cpu/Dispatch/Dispatch.hpp"

#include <algorithm>
#include <cstring>
#include <format>

using namespace bee;

// ── 内部辅助：按 DType 将 double 标量填充到原始缓冲区 ──────────────────────────

namespace
{

// 模板：将 value 转换为类型 T 后填充 n 个元素
template <DType D>
auto fill_typed(void* data, std::size_t n, double value) -> void
{
    using T   = dtype_cpp_t<D>;
    auto* ptr = static_cast<T*>(data);
    T     fill_val;
    if constexpr (D == DType::Bool) {
        // Bool：非零值视为 true
        fill_val = static_cast<T>(value != 0.0);
    } else {
        fill_val = static_cast<T>(value);
    }
    std::fill_n(ptr, n, fill_val);
}

// 运行时分派：根据 DType 调用对应模板特化
auto dispatch_fill(DType dt, void* data, std::size_t n, double value) -> void
{
    switch (dt) {
    case DType::Bool: fill_typed<DType::Bool>(data, n, value); break;
    case DType::U8: fill_typed<DType::U8>(data, n, value); break;
    case DType::I32: fill_typed<DType::I32>(data, n, value); break;
    case DType::I64: fill_typed<DType::I64>(data, n, value); break;
    case DType::F32: fill_typed<DType::F32>(data, n, value); break;
    case DType::F64: fill_typed<DType::F64>(data, n, value); break;
    default: break;
    }
}

// arange 填充：逐元素写入 start + i * step 后转换为目标类型
template <DType D>
auto arange_typed(void* data, std::size_t n, int64_t start, int64_t step) -> void
{
    using T   = dtype_cpp_t<D>;
    auto* ptr = static_cast<T*>(data);
    for (std::size_t i = 0; i < n; ++i) {
        ptr[i] = static_cast<T>(start + static_cast<int64_t>(i) * step);
    }
}

auto dispatch_arange(DType dt, void* data, std::size_t n, int64_t start, int64_t step) -> void
{
    switch (dt) {
    case DType::U8: arange_typed<DType::U8>(data, n, start, step); break;
    case DType::I32: arange_typed<DType::I32>(data, n, start, step); break;
    case DType::I64: arange_typed<DType::I64>(data, n, start, step); break;
    case DType::F32: arange_typed<DType::F32>(data, n, start, step); break;
    case DType::F64: arange_typed<DType::F64>(data, n, start, step); break;
    default: break;
    }
}

// ── 内部辅助：将含 -1 占位符的 new_shape 解析为确定 shape ──────────────────────

auto resolve_shape(const Shape& new_shape, int64_t total_numel) -> Result<Shape>
{
    int     neg_count  = 0;
    int     neg_idx    = -1;
    int64_t known_prod = 1;

    for (int i = 0; i < static_cast<int>(new_shape.size()); ++i) {
        const int64_t d = new_shape[i];
        if (d == -1) {
            ++neg_count;
            neg_idx = i;
        } else if (d < 0) {
            return std::unexpected(make_error(std::format("shape 中含非法负维度 {}", d), Severity::Recoverable));
        } else {
            known_prod *= d;
        }
    }

    if (neg_count > 1)
        return std::unexpected(make_error("shape 中至多允许一个 -1", Severity::Recoverable));

    Shape resolved = new_shape;

    if (neg_count == 1) {
        if (known_prod == 0) {
            // 已知维度中有 0；只有 total_numel 也为 0 时可推断为 0
            if (total_numel != 0)
                return std::unexpected(make_error("无法从已知维度（含零）推断 -1 所在维度", Severity::Recoverable));
            resolved[neg_idx] = 0;
        } else {
            if (total_numel % known_prod != 0)
                return std::unexpected(make_error("元素总数无法被已知维度整除，无法推断 -1", Severity::Recoverable));
            resolved[neg_idx] = total_numel / known_prod;
        }
    }

    if (::bee::numel(resolved) != total_numel)
        return std::unexpected(
            make_error(std::format("reshape 元素数 {} 与原张量 {} 不匹配", ::bee::numel(resolved), total_numel), Severity::Recoverable)
        );

    return resolved;
}

// ── 内部辅助：将非连续 TensorImpl 按 stride 逐元素拷贝到已分配目标缓冲区 ──────
void contiguous_copy_into(void* dst, const TensorImpl& src, std::size_t elem_sz)
{
    const auto* src_base = static_cast<const uint8_t*>(src.storage->data());
    auto*       dst_ptr  = static_cast<uint8_t*>(dst);

    const int      nd       = static_cast<int>(src.shape.size());
    const int64_t  n        = src.numel();
    const Shape&   sh       = src.shape;
    const Strides& st       = src.strides;
    const int64_t  base_off = src.offset;

    // 按 C-order 枚举所有逻辑元素，依 stride 计算源偏移后逐元素拷贝
    std::vector<int64_t> idx(nd, 0);
    for (int64_t linear = 0; linear < n; ++linear) {
        int64_t src_off = base_off;
        for (int i = 0; i < nd; ++i)
            src_off += idx[i] * st[i];

        std::memcpy(dst_ptr + linear * static_cast<int64_t>(elem_sz), src_base + src_off * static_cast<int64_t>(elem_sz), elem_sz);

        // 推进多维索引（末尾维度最快）
        for (int i = nd - 1; i >= 0; --i) {
            ++idx[i];
            if (idx[i] < sh[i])
                break;
            idx[i] = 0;
        }
    }
}

} // namespace

namespace bee
{

// ── 构造器 ──────────────────────────────────────────────────────────────────

Tensor::Tensor(std::shared_ptr<TensorImpl> impl)
    : impl_(std::move(impl))
{
}

// ── 工厂 ────────────────────────────────────────────────────────────────────

auto Tensor::empty(Shape shape, DType dtype, Device device) -> Result<Tensor>
{
    // 校验 shape：负维度非法
    for (auto d : shape) {
        if (d < 0) {
            return std::unexpected(make_error(std::format("Tensor::empty: 非法的负维度 {} in shape", d), Severity::Recoverable));
        }
    }

    const int64_t     n      = ::bee::numel(shape);
    const std::size_t nbytes = static_cast<std::size_t>(n) * dtype_size(dtype);

    IAllocator& alloc =
        (device == Device::CUDA) ? static_cast<IAllocator&>(CudaAllocator::instance()) : static_cast<IAllocator&>(CpuAllocator::instance());
    auto storage_result = Storage::allocate(nbytes, alloc);
    if (!storage_result)
        return std::unexpected(std::move(storage_result.error()));

    auto ti     = std::make_shared<TensorImpl>();
    ti->storage = std::move(*storage_result);
    ti->dtype   = dtype;
    ti->strides = compute_contiguous_strides(shape);
    ti->shape   = std::move(shape);
    ti->offset  = 0;

    return Tensor(std::move(ti));
}

// ── zeros ────────────────────────────────────────────────────────────────────

auto Tensor::zeros(Shape shape, DType dtype, Device device) -> Result<Tensor>
{
    // 直接复用 empty 分配内存，再按设备 memset 全零
    Tensor t;
    BEE_TRY_ASSIGN(t, empty(shape, dtype, device));

    const int64_t     n      = t.numel();
    const std::size_t nbytes = static_cast<std::size_t>(n) * dtype_size(dtype);
    if (nbytes > 0) {
        if (device == Device::CUDA) {
            BEE_TRY(tensor::cuda::memset(t.data_ptr(), 0, nbytes));
        } else {
            std::memset(t.data_ptr(), 0, nbytes);
        }
    }

    return t;
}

// ── ones ─────────────────────────────────────────────────────────────────────

auto Tensor::ones(Shape shape, DType dtype, Device device) -> Result<Tensor>
{
    // 复用 full，以 1.0 填充
    return full(std::move(shape), dtype, 1.0, device);
}

// ── full ─────────────────────────────────────────────────────────────────────

auto Tensor::full(Shape shape, DType dtype, double value, Device device) -> Result<Tensor>
{
    if (!dtype_is_cpu_computable(dtype))
        return std::unexpected(
            make_error(std::format("Tensor::full: 扩展 dtype::{} 暂不支持 CPU 端填充（NotImplemented）", enum_to_name(dtype)), Severity::Recoverable)
        );

    // CUDA 设备暂不支持直接 fill；先在 CPU 构造再搬运到目标设备
    if (device == Device::CUDA) {
        Tensor cpu_t;
        BEE_TRY_ASSIGN(cpu_t, full(std::move(shape), dtype, value, Device::CPU));
        return cpu_t.to(Device::CUDA);
    }

    Tensor t;
    BEE_TRY_ASSIGN(t, empty(shape, dtype, device));

    const int64_t n = t.numel();
    if (n > 0)
        dispatch_fill(dtype, t.data_ptr(), static_cast<std::size_t>(n), value);

    return t;
}

// ── arange ───────────────────────────────────────────────────────────────────

auto Tensor::arange(int64_t start, int64_t end, int64_t step, DType dtype, Device device) -> Result<Tensor>
{
    // Bool 类型不支持
    if (dtype == DType::Bool)
        return std::unexpected(make_error("arange 不支持 Bool 类型", Severity::Recoverable));

    // step 为 0 非法
    if (step == 0)
        return std::unexpected(make_error("arange: step 不能为 0", Severity::Recoverable));

    // 区间方向校验
    if (step > 0 && start > end)
        return std::unexpected(make_error(std::format("arange: step>0 但 start({}) > end({})", start, end), Severity::Recoverable));

    if (step < 0 && start < end)
        return std::unexpected(make_error(std::format("arange: step<0 但 start({}) < end({})", start, end), Severity::Recoverable));

    // 计算元素数（PyTorch 语义：ceil 除法）
    int64_t n = 0;
    if (start != end) {
        if (step > 0)
            n = (end - start + step - 1) / step;
        else
            n = (end - start + step + 1) / step;
    }

    // CUDA 设备暂不支持直接 fill；先在 CPU 构造再搬运到目标设备
    if (device == Device::CUDA) {
        Tensor cpu_t;
        BEE_TRY_ASSIGN(cpu_t, arange(start, end, step, dtype, Device::CPU));
        return cpu_t.to(Device::CUDA);
    }

    // 分配并填充
    Tensor t;
    BEE_TRY_ASSIGN(t, empty({n}, dtype, device));

    if (n > 0)
        dispatch_arange(dtype, t.data_ptr(), static_cast<std::size_t>(n), start, step);

    return t;
}

// ── 查询访问器 ───────────────────────────────────────────────────────────────

auto Tensor::defined() const noexcept -> bool
{
    return impl_ != nullptr;
}

auto Tensor::ndim() const noexcept -> int64_t
{
    BEE_CHECK(impl_ != nullptr);
    return static_cast<int64_t>(impl_->shape.size());
}

auto Tensor::numel() const noexcept -> int64_t
{
    BEE_CHECK(impl_ != nullptr);
    return impl_->numel();
}

auto Tensor::shape() const noexcept -> const Shape&
{
    BEE_CHECK(impl_ != nullptr);
    return impl_->shape;
}

auto Tensor::strides() const noexcept -> const Strides&
{
    BEE_CHECK(impl_ != nullptr);
    return impl_->strides;
}

auto Tensor::dtype() const noexcept -> DType
{
    BEE_CHECK(impl_ != nullptr);
    return impl_->dtype;
}

auto Tensor::device() const noexcept -> Device
{
    BEE_CHECK(impl_ != nullptr);
    return impl_->storage->device();
}

auto Tensor::is_contiguous() const noexcept -> bool
{
    BEE_CHECK(impl_ != nullptr);
    return ::bee::is_contiguous(impl_->shape, impl_->strides);
}

auto Tensor::storage_offset() const noexcept -> int64_t
{
    BEE_CHECK(impl_ != nullptr);
    return impl_->offset;
}

auto Tensor::data_ptr() noexcept -> void*
{
    BEE_CHECK(impl_ != nullptr);
    auto* base = static_cast<uint8_t*>(impl_->storage->data());
    return base + impl_->offset * static_cast<int64_t>(dtype_size(impl_->dtype));
}

auto Tensor::data_ptr() const noexcept -> const void*
{
    BEE_CHECK(impl_ != nullptr);
    const auto* base = static_cast<const uint8_t*>(impl_->storage->data());
    return base + impl_->offset * static_cast<int64_t>(dtype_size(impl_->dtype));
}

auto Tensor::storage() const noexcept -> const std::shared_ptr<Storage>&
{
    BEE_CHECK(impl_ != nullptr);
    return impl_->storage;
}

auto Tensor::impl() const noexcept -> const std::shared_ptr<TensorImpl>&
{
    BEE_CHECK(impl_ != nullptr);
    return impl_;
}

// ── clone ────────────────────────────────────────────────────────────────────

auto Tensor::clone() const -> Result<Tensor>
{
    if (!defined())
        return std::unexpected(make_error("不能克隆未定义的 Tensor", Severity::Recoverable));

    // 只分配一次新 storage，按需选择拷贝路径
    const std::size_t elem_sz = dtype_size(impl_->dtype);
    const std::size_t nbytes  = static_cast<std::size_t>(numel()) * elem_sz;

    if (device() == Device::CUDA) {
        // 非连续时先在设备端物化为 contiguous，再深拷贝，复用 Task 3 的 strided_copy 路径
        if (!is_contiguous()) {
            Tensor cont;
            BEE_TRY_ASSIGN(cont, contiguous());
            return cont.clone();
        }

        auto storage_result = Storage::allocate(nbytes, impl_->storage->allocator());
        if (!storage_result)
            return std::unexpected(std::move(storage_result.error()));

        if (nbytes > 0)
            BEE_TRY(tensor::cuda::memcpy_d2d((*storage_result)->data(), data_ptr(), nbytes));

        auto ti     = std::make_shared<TensorImpl>();
        ti->storage = std::move(*storage_result);
        ti->dtype   = impl_->dtype;
        ti->shape   = impl_->shape;
        ti->strides = compute_contiguous_strides(impl_->shape);
        ti->offset  = 0;
        return Tensor(std::move(ti));
    }

    auto storage_result = Storage::allocate(nbytes, impl_->storage->allocator());
    if (!storage_result)
        return std::unexpected(std::move(storage_result.error()));

    if (is_contiguous()) {
        // 连续：直接内存块拷贝
        std::memcpy((*storage_result)->data(), data_ptr(), nbytes);
    } else {
        // 非连续：stride-loop 拷贝，避免中间临时 contiguous 分配
        contiguous_copy_into((*storage_result)->data(), *impl_, elem_sz);
    }

    auto ti     = std::make_shared<TensorImpl>();
    ti->storage = std::move(*storage_result);
    ti->dtype   = impl_->dtype;
    ti->shape   = impl_->shape;
    ti->strides = compute_contiguous_strides(impl_->shape);
    ti->offset  = 0;

    return Tensor(std::move(ti));
}

// ── to ───────────────────────────────────────────────────────────────────────

auto Tensor::to(Device target) const -> Result<Tensor>
{
    if (!defined())
        return std::unexpected(make_error("不能在未定义 Tensor 上调用 to()", Severity::Recoverable));

    // 同设备：返回浅拷贝（共享 storage，零拷贝）
    if (device() == target)
        return *this;

    // 非连续源先 contiguous 化
    Tensor src = *this;
    if (!is_contiguous()) {
        BEE_TRY_ASSIGN(src, contiguous());
    }

    Tensor dst;
    BEE_TRY_ASSIGN(dst, empty(src.shape(), src.dtype(), target));

    const std::size_t nbytes = static_cast<std::size_t>(src.numel()) * dtype_size(src.dtype());
    if (nbytes > 0) {
        const Device from = src.device();
        const Device to   = target;
        if (from == Device::CPU && to == Device::CUDA) {
            BEE_TRY(tensor::cuda::memcpy_h2d(dst.data_ptr(), src.data_ptr(), nbytes));
        } else if (from == Device::CUDA && to == Device::CPU) {
            BEE_TRY(tensor::cuda::memcpy_d2h(dst.data_ptr(), src.data_ptr(), nbytes));
        } else if (from == Device::CUDA && to == Device::CUDA) {
            BEE_TRY(tensor::cuda::memcpy_d2d(dst.data_ptr(), src.data_ptr(), nbytes));
        } else {
            // CPU→CPU 在 device == target 时已处理；此处不可达
            return std::unexpected(make_error("Tensor::to 遇到未支持的设备组合", Severity::Recoverable));
        }
    }

    return dst;
}

auto Tensor::to(Device target, const tensor::cuda::ExecContext* exec_context) const -> Result<Tensor>
{
    // ctx == nullptr 时委托旧版同步 to()
    if (!exec_context)
        return to(target);

    if (!defined())
        return std::unexpected(make_error("不能在未定义 Tensor 上调用 to()", Severity::Recoverable));

    // 同设备：返回浅拷贝（共享 storage，零拷贝）
    if (device() == target)
        return *this;

    // 非连续源先 contiguous 化（传入 ctx，供 CUDA 路径使用）
    Tensor src = *this;
    if (!is_contiguous()) {
        BEE_TRY_ASSIGN(src, contiguous(exec_context));
    }

    Tensor dst;
    BEE_TRY_ASSIGN(dst, empty(src.shape(), src.dtype(), target));

    const std::size_t nbytes = static_cast<std::size_t>(src.numel()) * dtype_size(src.dtype());
    if (nbytes > 0) {
        const Device from = src.device();
        const Device to   = target;

        // 使用异步 memcpy bridge，传入 stream 参数
        if (from == Device::CPU && to == Device::CUDA) {
            BEE_TRY(tensor::cuda::memcpy_h2d_async(dst.data_ptr(), src.data_ptr(), nbytes, exec_context->stream));
        } else if (from == Device::CUDA && to == Device::CPU) {
            BEE_TRY(tensor::cuda::memcpy_d2h_async(dst.data_ptr(), src.data_ptr(), nbytes, exec_context->stream));
        } else if (from == Device::CUDA && to == Device::CUDA) {
            BEE_TRY(tensor::cuda::memcpy_d2d_async(dst.data_ptr(), src.data_ptr(), nbytes, exec_context->stream));
        } else {
            return std::unexpected(make_error("Tensor::to 遇到未支持的设备组合", Severity::Recoverable));
        }

        // 保持同步可观察语义，确保测试稳定通过
        BEE_TRY(tensor::cuda::synchronize());
    }

    return dst;
}

// ── contiguous ───────────────────────────────────────────────────────────────

auto Tensor::contiguous(const tensor::cuda::ExecContext* /*ctx*/) const -> Result<Tensor>
{
    if (is_contiguous())
        return *this; // 共享 storage，引用计数递增

    if (device() == Device::CUDA) {
        // 通用设备端物化：使用 strided_copy kernel，不再回退到 CPU。
        // 该路径覆盖所有步长布局（2D transpose、3D permute、非零 offset 等）。
        const int64_t     n       = numel();
        const std::size_t elem_sz = dtype_size(impl_->dtype);
        const std::size_t nbytes  = static_cast<std::size_t>(n) * elem_sz;

        auto storage_result = Storage::allocate(nbytes, impl_->storage->allocator());
        if (!storage_result)
            return std::unexpected(std::move(storage_result.error()));

        auto rc = tensor::cuda::strided_copy(
            static_cast<int>(impl_->dtype),
            impl_->storage->data(),        // storage 基地址
            (*storage_result)->data(),     // 连续目标缓冲
            impl_->shape.data(),
            impl_->strides.data(),
            static_cast<int>(impl_->shape.size()),
            impl_->offset,                 // 元素单位的 storage 偏移
            static_cast<std::size_t>(n)
        );
        if (!rc)
            return std::unexpected(std::move(rc.error()));

        auto ti     = std::make_shared<TensorImpl>();
        ti->storage = std::move(*storage_result);
        ti->dtype   = impl_->dtype;
        ti->shape   = impl_->shape;
        ti->strides = compute_contiguous_strides(impl_->shape);
        ti->offset  = 0;
        return Tensor(std::move(ti));
    }

    const int64_t     n       = numel();
    const std::size_t elem_sz = dtype_size(impl_->dtype);
    const std::size_t nbytes  = static_cast<std::size_t>(n) * elem_sz;

    auto storage_result = Storage::allocate(nbytes, impl_->storage->allocator());
    if (!storage_result)
        return std::unexpected(std::move(storage_result.error()));

    // B11 CPU fast-path：2D 情形走 blocked-tile 拷贝（F32 AVX2 还会进 8×8 寄存器转置）
    if (impl_->shape.size() == 2) {
        const auto*   src_base  = static_cast<const uint8_t*>(impl_->storage->data());
        const int64_t off_bytes = impl_->offset * static_cast<int64_t>(elem_sz);
        BEE_RT_DISPATCH_STMT(
            tr_copy_2d,
            src_base + off_bytes,
            (*storage_result)->data(),
            impl_->shape[0],
            impl_->shape[1],
            impl_->strides[0],
            impl_->strides[1],
            elem_sz
        );
    } else {
        // 通用 stride-loop 拷贝
        contiguous_copy_into((*storage_result)->data(), *impl_, elem_sz);
    }

    auto ti     = std::make_shared<TensorImpl>();
    ti->storage = std::move(*storage_result);
    ti->dtype   = impl_->dtype;
    ti->shape   = impl_->shape;
    ti->strides = compute_contiguous_strides(impl_->shape);
    ti->offset  = 0;

    return Tensor(std::move(ti));
}

// ── view ─────────────────────────────────────────────────────────────────────

auto Tensor::view(Shape new_shape) const -> Result<Tensor>
{
    if (!is_contiguous())
        return std::unexpected(make_error("view requires contiguous tensor, use reshape instead", Severity::Recoverable));

    Shape resolved;
    BEE_TRY_ASSIGN(resolved, resolve_shape(new_shape, numel()));

    auto ti     = std::make_shared<TensorImpl>();
    ti->storage = impl_->storage; // 零拷贝：共享 storage
    ti->dtype   = impl_->dtype;
    ti->shape   = std::move(resolved);
    ti->strides = compute_contiguous_strides(ti->shape);
    ti->offset  = impl_->offset;

    return Tensor(std::move(ti));
}

// ── reshape ──────────────────────────────────────────────────────────────────

auto Tensor::reshape(Shape new_shape) const -> Result<Tensor>
{
    if (is_contiguous())
        return view(std::move(new_shape));

    // 非连续：先整理为 contiguous（分配新 storage），再 view
    Tensor cont;
    BEE_TRY_ASSIGN(cont, contiguous());
    return cont.view(std::move(new_shape));
}

// ── permute ──────────────────────────────────────────────────────────────────

auto Tensor::permute(std::span<const int> dims) const -> Result<Tensor>
{
    const int64_t nd = ndim();

    if (static_cast<int64_t>(dims.size()) != nd)
        return std::unexpected(make_error(std::format("permute: 给定 {} 个维度，张量 ndim 为 {}", dims.size(), nd), Severity::Recoverable));

    // 验证：值在 [0, ndim)，无重复
    std::vector<bool> seen(static_cast<std::size_t>(nd), false);
    for (const int d : dims) {
        if (d < 0 || d >= nd)
            return std::unexpected(make_error(std::format("permute: 维度索引 {} 超出范围 [0, {})", d, nd), Severity::Recoverable));
        if (seen[static_cast<std::size_t>(d)])
            return std::unexpected(make_error(std::format("permute: 维度索引 {} 重复", d), Severity::Recoverable));
        seen[static_cast<std::size_t>(d)] = true;
    }

    Shape   new_shape(static_cast<std::size_t>(nd));
    Strides new_strides(static_cast<std::size_t>(nd));
    for (int i = 0; i < static_cast<int>(nd); ++i) {
        new_shape[static_cast<std::size_t>(i)]   = impl_->shape[static_cast<std::size_t>(dims[i])];
        new_strides[static_cast<std::size_t>(i)] = impl_->strides[static_cast<std::size_t>(dims[i])];
    }

    auto ti     = std::make_shared<TensorImpl>();
    ti->storage = impl_->storage;
    ti->dtype   = impl_->dtype;
    ti->shape   = std::move(new_shape);
    ti->strides = std::move(new_strides);
    ti->offset  = impl_->offset;

    return Tensor(std::move(ti));
}

auto Tensor::permute(std::initializer_list<int> dims) const -> Result<Tensor>
{
    return permute(std::span<const int>(dims.begin(), dims.size()));
}

// ── transpose ────────────────────────────────────────────────────────────────

auto Tensor::transpose(int dim0, int dim1) const -> Result<Tensor>
{
    const int nd = static_cast<int>(ndim());

    // 支持负数索引
    if (dim0 < 0)
        dim0 += nd;
    if (dim1 < 0)
        dim1 += nd;

    if (dim0 < 0 || dim0 >= nd || dim1 < 0 || dim1 >= nd)
        return std::unexpected(
            make_error(std::format("transpose: 维度索引越界（dim0={}, dim1={}, ndim={}）", dim0, dim1, nd), Severity::Recoverable)
        );

    // 拷贝 TensorImpl（shared_ptr<Storage> 共享所有权，shape/strides 深拷贝）
    auto ti = std::make_shared<TensorImpl>(*impl_);
    std::swap(ti->shape[static_cast<std::size_t>(dim0)], ti->shape[static_cast<std::size_t>(dim1)]);
    std::swap(ti->strides[static_cast<std::size_t>(dim0)], ti->strides[static_cast<std::size_t>(dim1)]);

    return Tensor(std::move(ti));
}

// ── squeeze ──────────────────────────────────────────────────────────────────

auto Tensor::squeeze(int dim) const -> Result<Tensor>
{
    const int nd = static_cast<int>(ndim());

    if (dim < 0)
        dim += nd;

    if (dim < 0 || dim >= nd)
        return std::unexpected(make_error(std::format("squeeze: 维度索引 {} 越界（ndim={}）", dim, nd), Severity::Recoverable));

    // 若该维度 size != 1，返回浅拷贝（语义与 PyTorch 一致，不报错）
    if (impl_->shape[static_cast<std::size_t>(dim)] != 1)
        return *this;

    Shape   new_shape;
    Strides new_strides;
    new_shape.reserve(static_cast<std::size_t>(nd - 1));
    new_strides.reserve(static_cast<std::size_t>(nd - 1));

    for (int i = 0; i < nd; ++i) {
        if (i != dim) {
            new_shape.push_back(impl_->shape[static_cast<std::size_t>(i)]);
            new_strides.push_back(impl_->strides[static_cast<std::size_t>(i)]);
        }
    }

    auto ti     = std::make_shared<TensorImpl>();
    ti->storage = impl_->storage;
    ti->dtype   = impl_->dtype;
    ti->shape   = std::move(new_shape);
    ti->strides = std::move(new_strides);
    ti->offset  = impl_->offset;

    return Tensor(std::move(ti));
}

// ── unsqueeze ────────────────────────────────────────────────────────────────

auto Tensor::unsqueeze(int dim) const -> Result<Tensor>
{
    const int nd = static_cast<int>(ndim());

    // dim 有效范围 [-ndim-1, ndim]
    if (dim < -(nd + 1) || dim > nd)
        return std::unexpected(
            make_error(std::format("unsqueeze: 维度索引 {} 越界（有效范围 [{}, {}]）", dim, -(nd + 1), nd), Severity::Recoverable)
        );

    if (dim < 0)
        dim += (nd + 1);

    Shape   new_shape;
    Strides new_strides;
    new_shape.reserve(static_cast<std::size_t>(nd + 1));
    new_strides.reserve(static_cast<std::size_t>(nd + 1));

    for (int i = 0; i <= nd; ++i) {
        if (i == dim) {
            new_shape.push_back(1);
            // stride：取右邻维度的步长；若 dim == nd（末尾插入）则取 1
            const int64_t s = (dim < nd) ? impl_->strides[static_cast<std::size_t>(dim)] : int64_t{1};
            new_strides.push_back(s);
        } else {
            const int orig = (i < dim) ? i : i - 1;
            new_shape.push_back(impl_->shape[static_cast<std::size_t>(orig)]);
            new_strides.push_back(impl_->strides[static_cast<std::size_t>(orig)]);
        }
    }

    auto ti     = std::make_shared<TensorImpl>();
    ti->storage = impl_->storage;
    ti->dtype   = impl_->dtype;
    ti->shape   = std::move(new_shape);
    ti->strides = std::move(new_strides);
    ti->offset  = impl_->offset;

    return Tensor(std::move(ti));
}

// ── slice ────────────────────────────────────────────────────────────────────

auto Tensor::slice(int dim, int64_t start, int64_t end, int64_t step) const -> Result<Tensor>
{
    const int nd = static_cast<int>(ndim());

    if (dim < 0)
        dim += nd;

    if (dim < 0 || dim >= nd)
        return std::unexpected(make_error(std::format("slice: 维度索引 {} 越界（ndim={}）", dim, nd), Severity::Recoverable));

    if (step < 1)
        return std::unexpected(make_error(std::format("slice: step={} 非法，必须 >= 1", step), Severity::Recoverable));

    const int64_t dim_size = impl_->shape[static_cast<std::size_t>(dim)];

    // 要求 0 <= start <= end <= dim_size（不做负索引处理）
    if (start < 0 || end < start || end > dim_size)
        return std::unexpected(
            make_error(std::format("slice: [start={}, end={}) 在维度 {} (size={}) 上越界", start, end, dim, dim_size), Severity::Recoverable)
        );

    // ceil((end - start) / step)
    const int64_t new_dim_size = (end - start + step - 1) / step;

    Shape   new_shape                          = impl_->shape;
    Strides new_strides                        = impl_->strides;
    new_shape[static_cast<std::size_t>(dim)]   = new_dim_size;
    new_strides[static_cast<std::size_t>(dim)] = impl_->strides[static_cast<std::size_t>(dim)] * step;

    const int64_t new_offset = impl_->offset + start * impl_->strides[static_cast<std::size_t>(dim)];

    auto ti     = std::make_shared<TensorImpl>();
    ti->storage = impl_->storage;
    ti->dtype   = impl_->dtype;
    ti->shape   = std::move(new_shape);
    ti->strides = std::move(new_strides);
    ti->offset  = new_offset;

    return Tensor(std::move(ti));
}

} // namespace bee
