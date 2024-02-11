#pragma once

#include "aabb.h"
#include "collider.h"
#include "collision.h"
#include "growable_array.h"
#include "settings.h"

#define muliNullNode (-1)

namespace muli
{

// You can use either Area() or Perimeter() as a surface area heuristic(SAH) function
inline float SAH(const AABB& aabb)
{
#if 1
    return aabb.GetArea();
#else
    return aabb.GetPerimeter();
#endif
}

typedef int32 NodeProxy;
typedef Collider Data;

class AABBTree
{
public:
    struct Node
    {
        bool IsLeaf() const
        {
            return child1 == muliNullNode;
        }

        int32 id;
        AABB aabb;

        NodeProxy parent;
        NodeProxy child1;
        NodeProxy child2;

        NodeProxy next;
        bool moved;

        Data* data; // user data
    };

    AABBTree();
    ~AABBTree() noexcept;

    AABBTree(const AABBTree&) = delete;
    AABBTree& operator=(const AABBTree&) = delete;

    AABBTree(AABBTree&&) noexcept;
    AABBTree& operator=(AABBTree&&) noexcept;

    void Reset();

    NodeProxy CreateNode(Data* data, const AABB& aabb);
    bool MoveNode(NodeProxy node, AABB aabb, const Vec2& displacement, bool forceMove);
    void RemoveNode(NodeProxy node);

    bool TestOverlap(NodeProxy nodeA, NodeProxy nodeB) const;
    const AABB& GetAABB(NodeProxy node) const;
    void ClearMoved(NodeProxy node) const;
    bool WasMoved(NodeProxy node) const;
    Data* GetData(NodeProxy node) const;

    template <typename T>
    void Traverse(T* callback) const;
    template <typename T>
    void Query(const Vec2& point, T* callback) const;
    template <typename T>
    void Query(const AABB& aabb, T* callback) const;
    template <typename T>
    void RayCast(const RayCastInput& input, T* callback) const;
    template <typename T>
    void AABBCast(const AABBCastInput& input, T* callback) const;

    void Traverse(const std::function<void(const Node*)>& callback) const;
    void Query(const Vec2& point, const std::function<bool(NodeProxy, Data*)>& callback) const;
    void Query(const AABB& aabb, const std::function<bool(NodeProxy, Data*)>& callback) const;
    void RayCast(const RayCastInput& input, const std::function<float(const RayCastInput& input, Data* data)>& callback) const;
    void AABBCast(const AABBCastInput& input, const std::function<float(const AABBCastInput& input, Data* data)>& callback) const;

    float ComputeTreeCost() const;
    void Rebuild();

private:
    NodeProxy nodeID;
    NodeProxy root;

    Node* nodes;
    int32 nodeCapacity;
    int32 nodeCount;

    NodeProxy freeList;

    NodeProxy AllocateNode();
    void FreeNode(NodeProxy node);

    NodeProxy InsertLeaf(NodeProxy leaf);
    void RemoveLeaf(NodeProxy leaf);

    void Rotate(NodeProxy node);
    void Swap(NodeProxy node1, NodeProxy node2);
};

inline bool AABBTree::TestOverlap(NodeProxy nodeA, NodeProxy nodeB) const
{
    muliAssert(0 <= nodeA && nodeA < nodeCapacity);
    muliAssert(0 <= nodeB && nodeB < nodeCapacity);

    return nodes[nodeA].aabb.TestOverlap(nodes[nodeB].aabb);
}

inline const AABB& AABBTree::GetAABB(NodeProxy node) const
{
    muliAssert(0 <= node && node < nodeCapacity);

    return nodes[node].aabb;
}

inline void AABBTree::ClearMoved(NodeProxy node) const
{
    muliAssert(0 <= node && node < nodeCapacity);

    nodes[node].moved = false;
}

inline bool AABBTree::WasMoved(NodeProxy node) const
{
    muliAssert(0 <= node && node < nodeCapacity);

    return nodes[node].moved;
}

inline Data* AABBTree::GetData(NodeProxy node) const
{
    muliAssert(0 <= node && node < nodeCapacity);

    return nodes[node].data;
}

inline float AABBTree::ComputeTreeCost() const
{
    float cost = 0.0f;

    Traverse([&cost](const Node* node) -> void { cost += SAH(node->aabb); });

    return cost;
}

template <typename T>
void AABBTree::Traverse(T* callback) const
{
    if (root == muliNullNode)
    {
        return;
    }

    GrowableArray<NodeProxy, 256> stack;
    stack.EmplaceBack(root);

    while (stack.Count() != 0)
    {
        NodeProxy current = stack.PopBack();

        if (nodes[current].IsLeaf() == false)
        {
            stack.EmplaceBack(nodes[current].child1);
            stack.EmplaceBack(nodes[current].child2);
        }

        const Node* node = nodes + current;
        callback->TraverseCallback(node);
    }
}

template <typename T>
void AABBTree::Query(const Vec2& point, T* callback) const
{
    if (root == muliNullNode)
    {
        return;
    }

    GrowableArray<NodeProxy, 256> stack;
    stack.EmplaceBack(root);

    while (stack.Count() != 0)
    {
        NodeProxy current = stack.PopBack();

        if (nodes[current].aabb.TestPoint(point) == false)
        {
            continue;
        }

        if (nodes[current].IsLeaf())
        {
            bool proceed = callback->QueryCallback(current, nodes[current].data);
            if (proceed == false)
            {
                return;
            }
        }
        else
        {
            stack.EmplaceBack(nodes[current].child1);
            stack.EmplaceBack(nodes[current].child2);
        }
    }
}

template <typename T>
void AABBTree::Query(const AABB& aabb, T* callback) const
{
    if (root == muliNullNode)
    {
        return;
    }

    GrowableArray<NodeProxy, 256> stack;
    stack.EmplaceBack(root);

    while (stack.Count() != 0)
    {
        NodeProxy current = stack.PopBack();

        if (nodes[current].aabb.TestOverlap(aabb) == false)
        {
            continue;
        }

        if (nodes[current].IsLeaf())
        {
            bool proceed = callback->QueryCallback(current, nodes[current].data);
            if (proceed == false)
            {
                return;
            }
        }
        else
        {
            stack.EmplaceBack(nodes[current].child1);
            stack.EmplaceBack(nodes[current].child2);
        }
    }
}

template <typename T>
void AABBTree::RayCast(const RayCastInput& input, T* callback) const
{
    Vec2 p1 = input.from;
    Vec2 p2 = input.to;
    float maxFraction = input.maxFraction;
    Vec2 radius(input.radius, input.radius);

    Vec2 d = p2 - p1;
    float length = d.NormalizeSafe();
    if (length == 0.0f)
    {
        return;
    }

    Vec2 perp = Cross(d, 1.0f); // separating axis
    Vec2 absPerp = Abs(perp);

    Vec2 end = p1 + maxFraction * (p2 - p1);
    AABB rayAABB;
    rayAABB.min = Min(p1, end) - radius;
    rayAABB.max = Max(p1, end) + radius;

    GrowableArray<NodeProxy, 256> stack;
    stack.EmplaceBack(root);

    while (stack.Count() > 0)
    {
        NodeProxy current = stack.PopBack();
        if (current == muliNullNode)
        {
            continue;
        }

        const Node* node = nodes + current;
        if (node->aabb.TestOverlap(rayAABB) == false)
        {
            continue;
        }

        Vec2 center = (node->aabb.min + node->aabb.max) * 0.5f;
        Vec2 extents = (node->aabb.max - node->aabb.min) * 0.5f;

        float separation = Abs(Dot(perp, p1 - center)) - Dot(absPerp, extents);
        if (separation > radius.x) // Separating axis test
        {
            continue;
        }

        if (node->IsLeaf())
        {
            RayCastInput subInput;
            subInput.from = p1;
            subInput.to = p2;
            subInput.maxFraction = maxFraction;
            subInput.radius = radius.x;

            float newFraction = callback->RayCastCallback(subInput, node->data);
            if (newFraction == 0.0f)
            {
                return;
            }

            if (newFraction > 0.0f)
            {
                // Update ray AABB
                maxFraction = newFraction;
                Vec2 newEnd = p1 + maxFraction * (p2 - p1);
                rayAABB.min = Min(p1, newEnd) - radius;
                rayAABB.max = Max(p1, newEnd) + radius;
            }
        }
        else
        {
            stack.EmplaceBack(node->child1);
            stack.EmplaceBack(node->child2);
        }
    }
}

template <typename T>
void AABBTree::AABBCast(const AABBCastInput& input, T* callback) const
{
    Vec2 p1 = input.from;
    Vec2 p2 = input.to;
    float maxFraction = input.maxFraction;
    Vec2 half = input.extents;

    Vec2 d = p2 - p1;
    float length = d.NormalizeSafe();
    if (length == 0.0f)
    {
        return;
    }

    Vec2 perp = Cross(d, 1.0f); // separating axis
    Vec2 absPerp = Abs(perp);

    float r = Dot(absPerp, half);

    Vec2 end = p1 + maxFraction * (p2 - p1);
    AABB rayAABB;
    rayAABB.min = Min(p1, end) - half;
    rayAABB.max = Max(p1, end) + half;

    GrowableArray<NodeProxy, 256> stack;
    stack.EmplaceBack(root);

    while (stack.Count() > 0)
    {
        NodeProxy current = stack.PopBack();
        if (current == muliNullNode)
        {
            continue;
        }

        const Node* node = nodes + current;
        if (node->aabb.TestOverlap(rayAABB) == false)
        {
            continue;
        }

        Vec2 center = (node->aabb.min + node->aabb.max) * 0.5f;
        Vec2 extents = (node->aabb.max - node->aabb.min) * 0.5f;

        float separation = Abs(Dot(perp, p1 - center)) - Dot(absPerp, extents);
        if (separation > r) // Separating axis test
        {
            continue;
        }

        if (node->IsLeaf())
        {
            AABBCastInput subInput;
            subInput.from = p1;
            subInput.to = p2;
            subInput.maxFraction = maxFraction;
            subInput.extents = half;

            float newFraction = callback->AABBCastCallback(subInput, node->data);
            if (newFraction == 0.0f)
            {
                return;
            }

            if (newFraction > 0.0f)
            {
                // Update ray AABB
                maxFraction = newFraction;
                Vec2 newEnd = p1 + maxFraction * (p2 - p1);
                rayAABB.min = Min(p1, newEnd) - half;
                rayAABB.max = Max(p1, newEnd) + half;
            }
        }
        else
        {
            stack.EmplaceBack(node->child1);
            stack.EmplaceBack(node->child2);
        }
    }
}

} // namespace muli