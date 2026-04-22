#include "Tensor/Ops/Broadcast.hpp"

#include <algorithm>
#include <format>

namespace bee
{

auto compute_broadcast_shape(const Shape& a, const Shape& b) -> Result<Shape>
{
    const int ra = static_cast<int>(a.size());
    const int rb = static_cast<int>(b.size());
    const int r  = std::max(ra, rb);

    Shape result(static_cast<std::size_t>(r));

    for (int i = 0; i < r; ++i) {
        // 从右对齐：计算当前维在各 shape 中的实际索引
        const int     ia = i - (r - ra);
        const int     ib = i - (r - rb);
        const int64_t da = (ia >= 0) ? a[static_cast<std::size_t>(ia)] : 1;
        const int64_t db = (ib >= 0) ? b[static_cast<std::size_t>(ib)] : 1;

        if (da == db) {
            result[static_cast<std::size_t>(i)] = da;
        } else if (da == 1) {
            result[static_cast<std::size_t>(i)] = db;
        } else if (db == 1) {
            result[static_cast<std::size_t>(i)] = da;
        } else {
            return std::unexpected(make_error(std::format("shape 不兼容，无法广播：对齐后第 {} 维大小 {} vs {}", i, da, db), Severity::Recoverable));
        }
    }

    return result;
}

} // namespace bee
