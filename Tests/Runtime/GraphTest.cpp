/**
 * @File GraphTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/22
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <Utility/Graph/Graph.hpp>

using namespace bee;

TEST(VertexTest, InvalidVertexId)
{
    EXPECT_EQ(kInvalidVertexId, static_cast<VertexId>(~0ull));
}

TEST(EdgeTest, DefaultConstructor)
{
    EdgeId e;
    EXPECT_EQ(e.va, kInvalidVertexId);
    EXPECT_EQ(e.vb, kInvalidVertexId);
}

TEST(EdgeTest, IsValid)
{
    EdgeId e{1, 2};
    EXPECT_TRUE(IsEdgeValid(e));
    e.va = kInvalidVertexId;
    EXPECT_FALSE(IsEdgeValid(e));
}

TEST(EdgeTest, MakeEdgeSorted)
{
    EdgeId e1 = MakeEdgeSorted(2, 1);
    EXPECT_EQ(e1.va, 1);
    EXPECT_EQ(e1.vb, 2);
    EdgeId e2 = MakeEdgeSorted(1, 2);
    EXPECT_EQ(e2.va, 1);
    EXPECT_EQ(e2.vb, 2);
}

class TestWeightEdge : public WeightedEdge<float>
{
public:
    BEE_NODISCARD float weight() const noexcept override { return 42.0; }
};

class TestWeightEdge2
{
public:
};

TEST(WeightedEdgeTest, Weight)
{
    TestWeightEdge tw;
    EXPECT_EQ(EdgeWeight(tw), 42);

    EXPECT_EQ(EdgeWeight(3), 3);
    int a = 7;
    EXPECT_EQ(EdgeWeight(a), 7);

    TestWeightEdge2 tw2;
    EXPECT_EQ(EdgeWeight(tw2), 1);
}

TEST(GraphTest, GraphType)
{
    DirectedGraph<int, int> directed;
    EXPECT_TRUE(directed.isDirected());
    EXPECT_FALSE(directed.isUndirected());

    UndirectedGraph<int, int> undirected;
    EXPECT_FALSE(undirected.isDirected());
    EXPECT_TRUE(undirected.isUndirected());
}

TEST(GraphTest, VertexCount)
{
    DirectedGraph<int, int> graph;

    EXPECT_EQ(graph.vertexCount(), 0);

    const auto vid_1{graph.addVertex(10)};
    EXPECT_EQ(graph.vertexCount(), 1);
    ASSERT_TRUE(graph.hasVertex(vid_1));
    EXPECT_EQ(graph.vertex(vid_1), 10);

    const auto vid_2{graph.addVertex(20)};
    EXPECT_EQ(graph.vertexCount(), 2);
    ASSERT_TRUE(graph.hasVertex(vid_2));
    EXPECT_EQ(graph.vertex(vid_2), 20);

    constexpr int id = 2;
    const auto vid{graph.addVertex(30, id)};
    EXPECT_EQ(graph.vertexCount(), 3);
    ASSERT_TRUE(graph.hasVertex(id));
    EXPECT_EQ(graph.vertex(id), 30);
}

TEST(GraphTest, AddAndRemoveVertex)
{
    DirectedGraph<int, int> graph;
    const auto vid_1{graph.addVertex(10)};
    const auto vid_2{graph.addVertex(20)};
    const auto vid_3{graph.addVertex(30)};

    graph.addEdge(vid_1, vid_2, 100);
    graph.addEdge(vid_1, vid_3, 200);

    EXPECT_EQ(graph.vertexCount(), 3);
    EXPECT_EQ(graph.edgeCount(), 2);

    graph.removeVertex(vid_1);
    EXPECT_EQ(graph.vertexCount(), 2);
    EXPECT_EQ(graph.edgeCount(), 0);
    ASSERT_FALSE(graph.hasVertex(vid_1));
    ASSERT_TRUE(graph.hasVertex(vid_2));
    ASSERT_TRUE(graph.hasVertex(vid_3));

    graph.removeVertex(vid_2);
    EXPECT_EQ(graph.vertexCount(), 1);
    ASSERT_FALSE(graph.hasVertex(vid_2));
    ASSERT_FALSE(graph.hasVertex(vid_1));
    ASSERT_TRUE(graph.hasVertex(vid_3));
    EXPECT_EQ(graph.edgeCount(), 0);
    ASSERT_FALSE(graph.hasEdge(vid_1, vid_2));
    ASSERT_FALSE(graph.hasEdge(vid_2, vid_3));

    const auto invalid = vid_1 + vid_3 + 1;
    graph.removeVertex(invalid);
    EXPECT_EQ(graph.vertexCount(), 1);
    ASSERT_FALSE(graph.hasVertex(invalid));
    ASSERT_FALSE(graph.hasVertex(vid_1));
    ASSERT_TRUE(graph.hasVertex(vid_3));
    EXPECT_EQ(graph.edgeCount(), 0);
    ASSERT_FALSE(graph.hasEdge(vid_1, invalid));
    ASSERT_FALSE(graph.hasEdge(invalid, vid_3));
}

TEST(GraphTest, HasVertex)
{
    DirectedGraph<int, int> graph;
    VertexId v = graph.addVertex(1);
    EXPECT_TRUE(graph.hasVertex(v));
    EXPECT_FALSE(graph.hasVertex(2));
}

TEST(GraphTest, Neighbors)
{
    DirectedGraph<int, int> graph;
    VertexId v1 = graph.addVertex(1);
    VertexId v2 = graph.addVertex(2);
    graph.addEdge(v1, v2, 1);
    EXPECT_EQ(graph.neighbors(v1).size(), 1);
    EXPECT_EQ(graph.neighbors(v1).count(v2), 1);
}

TEST(GraphTest, EdgeCount)
{
    DirectedGraph<int, int> graph;
    EXPECT_EQ(graph.edgeCount(), 0);
    auto v1 = graph.addVertex(1);
    auto v2 = graph.addVertex(1);
    graph.addEdge(v1, v2, 1);
    EXPECT_EQ(graph.edgeCount(), 1);
}

TEST(GraphTest, HasEdge)
{
    DirectedGraph<int, int> graph;
    auto v1 = graph.addVertex(1);
    auto v2 = graph.addVertex(1);
    graph.addEdge(v1, v2, 1);
    EXPECT_TRUE(graph.hasEdge(v1, v2));
    EXPECT_FALSE(graph.hasEdge(v2, v1));
}

TEST(GraphTest, AddAndRemoveEdge)
{
    DirectedGraph<int, int> graph;
    const auto vid_1{graph.addVertex(10)};
    const auto vid_2{graph.addVertex(20)};

    graph.addEdge(vid_1, vid_2, 100);
    EXPECT_EQ(graph.edgeCount(), 1);
    ASSERT_TRUE(graph.hasEdge(vid_1, vid_2));

    graph.removeEdge(vid_1, vid_2);

    EXPECT_EQ(graph.edgeCount(), 0);
    ASSERT_FALSE(graph.hasEdge(vid_1, vid_2));

    EXPECT_EQ(graph.vertexCount(), 2);
    ASSERT_TRUE(graph.hasVertex(vid_1));
    ASSERT_TRUE(graph.hasVertex(vid_2));
}

TEST(GraphTest, DumpDirected)
{
    DirectedGraph<std::string, int> graph;
    auto a = graph.addVertex("A");
    auto b = graph.addVertex("B");
    auto c = graph.addVertex("C");
    auto d = graph.addVertex("D");
    auto e = graph.addVertex("E");

    graph.addEdge(a, b, 1);
    graph.addEdge(a, c, 3);
    graph.addEdge(a, e, 4);

    graph.addEdge(c, d, 5);
    graph.addEdge(b, d, 6);
    graph.addEdge(b, e, 7);

    Dump(std::cout, graph);
}

TEST(GraphTest, VertexOutDegree)
{
    // GIVEN
    DirectedGraph<int, int> graph{};

    const auto vid_1{graph.addVertex(10)};
    const auto vid_2{graph.addVertex(20)};
    const auto vid_3{graph.addVertex(30)};
    const auto vid_4{graph.addVertex(40)};

    // WHEN
    graph.addEdge(vid_1, vid_2, 100);
    graph.addEdge(vid_2, vid_3, 200);
    graph.addEdge(vid_2, vid_4, 300);

    // THEN
    EXPECT_EQ(graph.outdegree(vid_1), 1);
    EXPECT_EQ(graph.outdegree(vid_2), 2);
    EXPECT_EQ(graph.outdegree(vid_3), 0);
    EXPECT_EQ(graph.outdegree(vid_4), 0);
}

TEST(GraphTest, VertexInDegree)
{
    // GIVEN
    DirectedGraph<int, int> graph{};

    const auto vid_1{graph.addVertex(10)};
    const auto vid_2{graph.addVertex(20)};
    const auto vid_3{graph.addVertex(30)};
    const auto vid_4{graph.addVertex(40)};

    // WHEN
    graph.addEdge(vid_1, vid_2, 100);
    graph.addEdge(vid_1, vid_3, 200);
    graph.addEdge(vid_3, vid_4, 300);
    graph.addEdge(vid_2, vid_4, 400);

    // THEN
    EXPECT_EQ(graph.indegree(vid_1), 0);
    EXPECT_EQ(graph.indegree(vid_2), 1);
    EXPECT_EQ(graph.indegree(vid_3), 1);
    EXPECT_EQ(graph.indegree(vid_4), 2);
}

TEST(GraphTest, VertexDegree)
{
    DirectedGraph<int, int> graph{};

    const auto vid_1{graph.addVertex(10)};
    const auto vid_2{graph.addVertex(20)};
    const auto vid_3{graph.addVertex(30)};
    const auto vid_4{graph.addVertex(40)};

    graph.addEdge(vid_1, vid_2, 100);
    graph.addEdge(vid_1, vid_3, 200);
    graph.addEdge(vid_3, vid_4, 300);
    graph.addEdge(vid_2, vid_4, 400);

    EXPECT_EQ(graph.degree(vid_1), 2);
    EXPECT_EQ(graph.degree(vid_2), 2);
    EXPECT_EQ(graph.degree(vid_3), 2);
    EXPECT_EQ(graph.degree(vid_4), 2);
}

TEST(UndirectedGraphPropertiesTest, VertexOutDegree)
{
    UndirectedGraph<int, int> graph{};

    const auto vid_1{graph.addVertex(10)};
    const auto vid_2{graph.addVertex(20)};
    const auto vid_3{graph.addVertex(30)};
    const auto vid_4{graph.addVertex(40)};

    graph.addEdge(vid_1, vid_2, 100);
    graph.addEdge(vid_2, vid_3, 200);
    graph.addEdge(vid_2, vid_4, 300);

    EXPECT_EQ(graph.outdegree(vid_1), 1);
    EXPECT_EQ(graph.outdegree(vid_2), 3);
    EXPECT_EQ(graph.outdegree(vid_3), 1);
    EXPECT_EQ(graph.outdegree(vid_4), 1);
}

TEST(UndirectedGraphPropertiesTest, VertexInDegree)
{
    UndirectedGraph<int, int> graph{};

    const auto vid_1{graph.addVertex(10)};
    const auto vid_2{graph.addVertex(20)};
    const auto vid_3{graph.addVertex(30)};
    const auto vid_4{graph.addVertex(40)};

    graph.addEdge(vid_1, vid_2, 100);
    graph.addEdge(vid_2, vid_3, 200);
    graph.addEdge(vid_2, vid_4, 300);

    EXPECT_EQ(graph.indegree(vid_1), 1);
    EXPECT_EQ(graph.indegree(vid_2), 3);
    EXPECT_EQ(graph.indegree(vid_3), 1);
    EXPECT_EQ(graph.indegree(vid_4), 1);
}

TEST(UndirectedGraphPropertiesTest, VertexDegree)
{
    UndirectedGraph<int, int> graph{};

    const auto vid_1{graph.addVertex(10)};
    const auto vid_2{graph.addVertex(20)};
    const auto vid_3{graph.addVertex(30)};
    const auto vid_4{graph.addVertex(40)};

    graph.addEdge(vid_1, vid_2, 100);
    graph.addEdge(vid_2, vid_3, 200);
    graph.addEdge(vid_2, vid_4, 300);

    EXPECT_EQ(graph.degree(vid_1), 1);
    EXPECT_EQ(graph.degree(vid_2), 3);
    EXPECT_EQ(graph.degree(vid_3), 1);
    EXPECT_EQ(graph.degree(vid_4), 1);
}
