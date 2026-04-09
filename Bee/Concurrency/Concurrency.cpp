#include "Concurrency/Concurrency.hpp"

#include "Base/Base.hpp"

namespace bee
{

std::string_view concurrency_name() noexcept
{
    return "Concurrency";
}

std::string_view concurrency_base_name() noexcept
{
    return base_name();
}

} // namespace bee
