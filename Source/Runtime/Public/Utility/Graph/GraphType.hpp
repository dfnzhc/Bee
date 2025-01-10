/**
 * @File GraphType.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/22
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Portability.hpp"
#include "Utility/Hash.hpp"
#include "Utility/Error.hpp"
#include <unordered_set>
#include <utility>
#include <ranges>
#include <algorithm>

namespace bee {

using VertexId                  = std::size_t;
constexpr auto kInvalidVertexId = static_cast<VertexId>(~0ull);

struct EdgeId
{
    VertexId va = {kInvalidVertexId};
    VertexId vb = {kInvalidVertexId};

    friend constexpr bool operator==(const EdgeId& lhs, const EdgeId& rhs) { return lhs.va == rhs.va && lhs.vb == rhs.vb; }
};

BEE_NODISCARD inline constexpr bool IsVertexValid(VertexId v)
{
    return v != kInvalidVertexId;
}

BEE_NODISCARD inline constexpr bool IsEdgeValid(EdgeId e)
{
    return IsVertexValid(e.va) && IsVertexValid(e.vb);
}

BEE_NODISCARD inline EdgeId MakeEdgeSorted(VertexId va, VertexId vb)
{
    if (va > vb)
        return {vb, va};
    return {va, vb};
}

struct EdgeIdHash
{
    BEE_NODISCARD std::size_t operator()(const EdgeId& e) const
    {
        std::size_t seed = 0;
        HashCombine(seed, e.va);
        HashCombine(seed, e.vb);

        return seed;
    }
};

// -------------------------
// Edge
// -------------------------

template<typename Weight = int> class WeightedEdge
{
public:
    using WeightType        = Weight;
    virtual ~WeightedEdge() = default;

    BEE_NODISCARD virtual Weight weight() const noexcept = 0;
};

template<typename derived>
concept WeightedEdgeType = std::is_base_of_v<WeightedEdge<typename derived::WeightType>, derived>;

template<typename WeightedEdge>
    requires WeightedEdgeType<WeightedEdge>
BEE_NODISCARD auto EdgeWeight(const WeightedEdge& edge)
{
    return edge.weight();
}

template<typename E>
    requires std::is_arithmetic_v<E>
BEE_NODISCARD E EdgeWeight(const E& edge)
{
    return edge;
}

template<typename E> BEE_NODISCARD int EdgeWeight(const E&)
{
    // default weight always 1
    return 1;
}

// -------------------------
// graph
// -------------------------

enum class GraphType
{
    Directed,
    Undirected
};

template<typename V, typename E, GraphType T> class Graph
{
public:
    using VertexType = V;
    using EdgeType   = E;

    using VerticesType      = std::unordered_set<VertexId>;
    using AdjacencyListType = std::unordered_map<VertexId, VerticesType>;

    using VerticesMap = std::unordered_map<VertexId, V>;
    using EdgesMap    = std::unordered_map<EdgeId, EdgeType, EdgeIdHash>;

    // clang-format off
    BEE_NODISCARD constexpr bool isDirected()   const { return T == GraphType::Directed; }
    BEE_NODISCARD constexpr bool isUndirected() const { return T == GraphType::Undirected; }

    // -------------------------
    // vertex
    // -------------------------

    BEE_NODISCARD std::size_t        vertexCount() const noexcept;
    BEE_NODISCARD const VerticesMap& vertices() const noexcept { return _vertices; }

    BEE_NODISCARD bool              hasVertex(VertexId vid) const noexcept;
    BEE_NODISCARD VertexType&       vertex(VertexId vid);
    BEE_NODISCARD const VertexType& vertex(VertexId vid) const;
    BEE_NODISCARD VerticesType      neighbors(VertexId vid) const;
    
    BEE_NODISCARD VertexId addVertex(auto&& vertex);
    BEE_NODISCARD VertexId addVertex(auto&& vertex, VertexId vid);

    BEE_NODISCARD std::size_t degree(VertexId vid) const;
    BEE_NODISCARD std::size_t outdegree(VertexId vid) const;
    BEE_NODISCARD std::size_t indegree(VertexId vid) const;

    void removeVertex(VertexId vid);

    // -------------------------
    // edge
    // -------------------------
    BEE_NODISCARD std::size_t     edgeCount() const noexcept;
    BEE_NODISCARD const EdgesMap& edges()  const noexcept { return _edges; }

    BEE_NODISCARD bool            hasEdge(VertexId va, VertexId vb) const noexcept;
    BEE_NODISCARD EdgeType&       edge(VertexId va, VertexId vb);
    BEE_NODISCARD EdgeType&       edge(const EdgeId& eid);
    BEE_NODISCARD const EdgeType& edge(VertexId va, VertexId vb) const;
    BEE_NODISCARD const EdgeType& edge(const EdgeId& eid) const;

    void addEdge(VertexId va, VertexId vb, auto&& edge);
    void removeEdge(VertexId va, VertexId vb);
    // clang-format on

private:
    VerticesMap _vertices            = {};
    EdgesMap _edges                  = {};
    AdjacencyListType _adjacencyList = {};

    std::size_t _nextVertexId = 0;
};

template<typename V, typename E> using DirectedGraph   = Graph<V, E, GraphType::Directed>;
template<typename V, typename E> using UndirectedGraph = Graph<V, E, GraphType::Undirected>;

template<typename V, typename E, GraphType T> std::size_t Graph<V, E, T>::vertexCount() const noexcept
{
    return _vertices.size();
}

template<typename V, typename E, GraphType T> bool Graph<V, E, T>::hasVertex(VertexId vid) const noexcept
{
    return _vertices.contains(vid);
}

template<typename V, typename E, GraphType T> V& Graph<V, E, T>::vertex(VertexId vid)
{
    return const_cast<V&>(const_cast<const Graph<V, E, T>*>(this)->vertex(vid));
}

template<typename V, typename E, GraphType T> const V& Graph<V, E, T>::vertex(VertexId vid) const
{
    BEE_DEBUG_ASSERT(hasVertex(vid), "No vertex [{}] in the graph.", vid);
    return _vertices.at(vid);
}

template<typename V, typename E, GraphType T> typename Graph<V, E, T>::VerticesType Graph<V, E, T>::neighbors(VertexId vid) const
{
    if (!_adjacencyList.contains(vid)) {
        return {};
    }
    return _adjacencyList.at(vid);
}

template<typename V, typename E, GraphType T> VertexId Graph<V, E, T>::addVertex(auto&& vertex)
{
    while (hasVertex(_nextVertexId)) {
        ++_nextVertexId;
    }
    const auto vid = _nextVertexId;
    _vertices.emplace(vid, std::forward<decltype(vertex)>(vertex));
    return vid;
}

template<typename V, typename E, GraphType T> VertexId Graph<V, E, T>::addVertex(auto&& vertex, VertexId vid)
{
    BEE_DEBUG_ASSERT(!hasVertex(vid), "Vertex [{}] already exists.", vid);
    _vertices.emplace(vid, std::forward<decltype(vertex)>(vertex));
    return vid;
}

template<typename V, typename E, GraphType T> void Graph<V, E, T>::removeVertex(VertexId vid)
{
    if (_adjacencyList.contains(vid)) {
        for (auto& target_vertex_id : _adjacencyList.at(vid)) {
            _edges.erase({vid, target_vertex_id});
        }
    }

    _adjacencyList.erase(vid);
    _vertices.erase(vid);

    for (auto& [source_vertex_id, neighbors] : _adjacencyList) {
        neighbors.erase(vid);
        _edges.erase({source_vertex_id, vid});
    }
}

template<typename V, typename E, GraphType T> std::size_t Graph<V, E, T>::degree(VertexId vid) const
{
    if constexpr (T == GraphType::Directed) {
        return outdegree(vid) + indegree(vid);
    }

    if constexpr (T == GraphType::Undirected) {
        return outdegree(vid);
    }
}

template<typename V, typename E, GraphType T> std::size_t Graph<V, E, T>::outdegree(VertexId vid) const
{
    return neighbors(vid).size();
}

template<typename V, typename E, GraphType T> std::size_t Graph<V, E, T>::indegree(VertexId vid) const
{
    if constexpr (T == GraphType::Directed) {
        return std::ranges::count_if(vertices(), [this, vid](const auto& kv_pair) {
            const auto& [current_vertex_id, _] = kv_pair;
            return neighbors(current_vertex_id).contains(vid);
        });
    }

    if constexpr (T == GraphType::Undirected) {
        return outdegree(vid);
    }

    BEE_UNREACHABLE();
}

template<typename V, typename E, GraphType T> std::size_t Graph<V, E, T>::edgeCount() const noexcept
{
    return _edges.size();
}

template<typename V, typename E, GraphType T> bool Graph<V, E, T>::hasEdge(VertexId va, VertexId vb) const noexcept
{
    if constexpr (T == GraphType::Directed) {
        return _edges.contains({va, vb});
    }
    else if constexpr (T == GraphType::Undirected) {
        return _edges.contains(MakeEdgeSorted(va, vb));
    }

    BEE_UNREACHABLE();
}

template<typename V, typename E, GraphType T> typename Graph<V, E, T>::EdgeType& Graph<V, E, T>::edge(VertexId va, VertexId vb)
{
    return const_cast<Graph<V, E, T>::EdgeType&>(const_cast<const Graph<V, E, T>*>(this)->edge(va, vb));
}

template<typename V, typename E, GraphType T> const typename Graph<V, E, T>::EdgeType& Graph<V, E, T>::edge(VertexId va, VertexId vb) const
{
    BEE_DEBUG_ASSERT(hasEdge(va, vb), "No edge between v[{}] -> v[{}].", va, vb);

    if constexpr (T == GraphType::Directed) {
        return _edges.at({va, vb});
    }
    else if constexpr (T == GraphType::Undirected) {
        return _edges.at(MakeEdgeSorted(va, vb));
    }

    BEE_UNREACHABLE();
}

template<typename V, typename E, GraphType T> typename Graph<V, E, T>::EdgeType& Graph<V, E, T>::edge(const EdgeId& eid)
{
    const auto [va, vb] = eid;
    return edge(va, vb);
}

template<typename V, typename E, GraphType T> const typename Graph<V, E, T>::EdgeType& Graph<V, E, T>::edge(const EdgeId& eid) const
{
    const auto [va, vb] = eid;
    return edge(va, vb);
}

template<typename V, typename E, GraphType T> void Graph<V, E, T>::addEdge(VertexId va, VertexId vb, auto&& edge)
{
    BEE_DEBUG_ASSERT(hasVertex(va), "Vertex [{}] not exists.", va);
    BEE_DEBUG_ASSERT(hasVertex(vb), "Vertex [{}] not exists.", vb);

    if constexpr (T == GraphType::Directed) {
        _adjacencyList[va].insert(vb);
        _edges.emplace(EdgeId{va, vb}, std::forward<decltype(edge)>(edge));
        return;
    }
    else if constexpr (T == GraphType::Undirected) {
        _adjacencyList[va].insert(vb);
        _adjacencyList[vb].insert(va);
        _edges.emplace(MakeEdgeSorted(va, vb), std::forward<decltype(edge)>(edge));
        return;
    }

    BEE_UNREACHABLE();
}

template<typename V, typename E, GraphType T> void Graph<V, E, T>::removeEdge(VertexId va, VertexId vb)
{
    if constexpr (T == GraphType::Directed) {
        _adjacencyList.at(va).erase(vb);
        _edges.erase({va, vb});
        return;
    }
    else if constexpr (T == GraphType::Undirected) {
        _adjacencyList.at(va).erase(vb);
        _adjacencyList.at(vb).erase(va);
        _edges.erase(MakeEdgeSorted(va, vb));
        return;
    }

    BEE_UNREACHABLE();
}

} // namespace bee