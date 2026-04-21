// SIMD 组件桩：提供最小化翻译单元，为 STATIC 库提供 TU；
// 下一阶段将在此处添加 CPUID 运行期检测逻辑
namespace bee::simd
{

// 返回组件名称（用于诊断输出）
const char* component_name() noexcept
{
    return "SIMD";
}

} // namespace bee::simd
