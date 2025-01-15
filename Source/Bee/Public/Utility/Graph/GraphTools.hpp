/**
 * @File GraphTools.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/22
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Utility/Graph/GraphType.hpp"

namespace bee {

namespace internal {

template<GraphType T> constexpr std::string_view DotHeadString()
{
    if constexpr (T == GraphType::Directed) {
        return "digraph";
    }
    else if constexpr (T == GraphType::Undirected) {
        return "graph";
    }

    BEE_UNREACHABLE();
}

template<GraphType T> constexpr std::string_view DotEdgeString()
{
    if constexpr (T == GraphType::Directed) {
        return "->";
    }
    else if constexpr (T == GraphType::Undirected) {
        return "--";
    }

    BEE_UNREACHABLE();
}

template<typename T>
    requires CanFormatType<T>
std::string VertexToDotString(VertexId, const T& vertex)
{
    return fmt::format("label=\"{}\"", vertex);
}

std::string EdgeToDotString(const EdgeId&, const auto& edge)
{
    return fmt::format("label=\"{}\"", EdgeWeight(edge));
}

} // namespace internal

template<typename V, typename E, GraphType T> void Dump(std::ostream& os, const Graph<V, E, T>& graph)
{
    os << internal::DotHeadString<T>() << " BeeGraph {\n";

    for (const auto& [vid, vertex] : graph.vertices()) {
        os << "\t" << std::to_string(vid) << " [" << internal::VertexToDotString(vid, vertex) << "];\n";
    }

    const auto edgeString = internal::DotEdgeString<T>();
    for (const auto& [eid, edge] : graph.edges()) {
        const auto [srcId, dstId] = eid;
        os << "\t" << std::to_string(srcId) << " " << edgeString << " " << std::to_string(dstId) << " [" << internal::EdgeToDotString(eid, edge)
           << "];\n";
    }

    os << "}\n";
}

} // namespace bee