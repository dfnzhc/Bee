#include "DataStructure/DataStructure.hpp"

#include "Base/Base.hpp"

namespace bee
{

std::string_view data_structure_name() noexcept
{
    return "DataStructure";
}

std::string_view data_structure_base_name() noexcept
{
    return base_name();
}

} // namespace bee
