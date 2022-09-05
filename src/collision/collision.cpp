#include "spe/collision.h"
#include "spe/box.h"
#include "spe/circle.h"
#include "spe/edge.h"
#include "spe/polygon.h"
#include "spe/polytope.h"
#include "spe/rigidbody.h"
#include "spe/simplex.h"

#define APPLY_AXIS_WEIGHT 1

namespace spe
{
static constexpr glm::vec2 weightAxis = { 0.0f, 1.0f };

struct SupportResult
{
    glm::vec2 vertex;
    int32_t index;
};

// Returns the fardest vertex in the 'dir' direction
// 'dir' should be normalized
static SupportResult support(RigidBody* b, glm::vec2 dir)
{
    RigidBody::Shape shape = b->GetShape();
    if (shape == RigidBody::Shape::ShapePolygon)
    {
        Polygon* p = static_cast<Polygon*>(b);
        const std::vector<glm::vec2>& vertices = p->GetVertices();

        int32_t idx = 0;
        float maxValue = glm::dot(dir, vertices[idx]);

        for (int32_t i = 1; i < vertices.size(); i++)
        {
            float value = glm::dot(dir, vertices[i]);
            if (value > maxValue)
            {
                idx = i;
                maxValue = value;
            }
        }

        return { vertices[idx], idx };
    }
    else if (shape == RigidBody::Shape::ShapeCircle)
    {
        Circle* c = static_cast<Circle*>(b);

        return { dir * c->GetRadius(), -1 };
    }
    else
    {
        throw std::exception("Not a supported shape");
    }
}

/*
 * Returns support point in 'Minkowski Difference' set
 * Minkowski Sum: A ⊕ B = {Pa + Pb| Pa ∈ A, Pb ∈ B}
 * Minkowski Difference : A ⊖ B = {Pa - Pb| Pa ∈ A, Pb ∈ B}
 * CSO stands for Configuration Space Object
 */
//'dir' should be normalized
static glm::vec2 cso_support(RigidBody* b1, RigidBody* b2, glm::vec2 dir)
{
    const glm::vec2 localDirP1 = glm::mul(b1->GlobalToLocal(), dir, 0);
    const glm::vec2 localDirP2 = glm::mul(b2->GlobalToLocal(), -dir, 0);

    const glm::vec2 supportP1 = b1->LocalToGlobal() * support(b1, localDirP1).vertex;
    const glm::vec2 supportP2 = b2->LocalToGlobal() * support(b2, localDirP2).vertex;

    return supportP1 - supportP2;
}

struct GJKResult
{
    bool collide;
    Simplex simplex;
};

static GJKResult gjk(RigidBody* b1, RigidBody* b2, bool earlyReturn = true)
{
    constexpr glm::vec2 origin{ 0.0f };
    glm::vec2 dir{ 1.0f, 0.0f }; // Random initial direction

    bool collide = false;
    Simplex simplex{};

    glm::vec2 supportPoint = cso_support(b1, b2, dir);
    simplex.AddVertex(supportPoint);

    for (uint32_t k = 0; k < GJK_MAX_ITERATION; k++)
    {
        ClosestResult closest = simplex.GetClosest(origin);

        if (glm::distance2(closest.point, origin) < GJK_TOLERANCE)
        {
            collide = true;
            break;
        }

        if (simplex.Count() != 1)
        {
            // Rebuild the simplex with vertices that are used(involved) to calculate closest distance
            simplex.Shrink(closest.contributors, closest.count);
        }

        dir = origin - closest.point;
        float dist = glm::length(dir);
        dir = glm::normalize(dir);
        supportPoint = cso_support(b1, b2, dir);

        // If the new support point is not further along the search direction than the closest point,
        // two objects are not colliding so you can early return here.
        if (earlyReturn && dist > glm::dot(dir, supportPoint - closest.point))
        {
            collide = false;
            break;
        }

        if (simplex.ContainsVertex(supportPoint))
        {
            collide = false;
            break;
        }
        else
        {
            simplex.AddVertex(supportPoint);
        }
    }

    return { collide, simplex };
}

struct EPAResult
{
    float penetrationDepth;
    glm::vec2 contactNormal;
};

static EPAResult epa(RigidBody* b1, RigidBody* b2, Simplex& gjkResult)
{
    Polytope polytope{ gjkResult };

    ClosestEdgeInfo closestEdge{ 0, FLT_MAX, glm::vec2{ 0.0f } };

    for (uint32_t i = 0; i < EPA_MAX_ITERATION; i++)
    {
        closestEdge = polytope.GetClosestEdge();
        const glm::vec2 supportPoint = cso_support(b1, b2, closestEdge.normal);
        const float newDistance = glm::dot(closestEdge.normal, supportPoint);

        if (glm::abs(closestEdge.distance - newDistance) > EPA_TOLERANCE)
        {
            // Insert the support vertex so that it expands our polytope
            polytope.vertices.Insert(closestEdge.index + 1, supportPoint);
        }
        else
        {
            // If you didn't expand edge, it means you reached the closest outer edge
            break;
        }
    }

    return { closestEdge.distance, closestEdge.normal };
}

static Edge find_farthest_edge(RigidBody* b, const glm::vec2& dir)
{
    const glm::vec2 localDir = glm::mul(b->GlobalToLocal(), dir, 0);
    const SupportResult farthest = support(b, localDir);

    glm::vec2 curr = farthest.vertex;
    int32_t idx = farthest.index;

    const glm::mat3 localToGlobal = b->LocalToGlobal();

    RigidBody::Shape shape = b->GetShape();
    if (shape == RigidBody::Shape::ShapeCircle)
    {
        curr = localToGlobal * curr;
        const glm::vec2 tangent = glm::cross(1.0f, dir) * TANGENT_MIN_LENGTH;

        return Edge{ curr, curr + tangent };
    }
    else if (shape == RigidBody::Shape::ShapePolygon)
    {
        Polygon* p = static_cast<Polygon*>(b);

        const std::vector<glm::vec2>& vertices = p->GetVertices();
        int32_t vertexCount = static_cast<int32_t>(p->VertexCount());

        const glm::vec2& prev = vertices[(idx - 1 + vertexCount) % vertexCount];
        const glm::vec2& next = vertices[(idx + 1) % vertexCount];

        const glm::vec2 e1 = glm::normalize(curr - prev);
        const glm::vec2 e2 = glm::normalize(curr - next);

        const bool w = glm::abs(glm::dot(e1, localDir)) <= glm::abs(glm::dot(e2, localDir));

        curr = localToGlobal * curr;

        return w ? Edge{ localToGlobal * prev, curr, (idx - 1 + vertexCount) % vertexCount, idx }
                 : Edge{ curr, localToGlobal * next, idx, (idx + 1) % vertexCount };
    }
    else
    {
        throw std::exception("Not a supported shape");
    }
}

static void clip_edge(Edge* edge, const glm::vec2& p, const glm::vec2& dir, bool removeClippedPoint = false)
{
    float d1 = glm::dot(edge->p1 - p, dir);
    float d2 = glm::dot(edge->p2 - p, dir);

    if (d1 >= 0 && d2 >= 0)
    {
        return;
    }

    float per = glm::abs(d1) + glm::abs(d2);

    if (d1 < 0)
    {
        if (removeClippedPoint)
        {
            edge->p1 = edge->p2;
            edge->id1 = edge->id2;
        }
        else
        {
            edge->p1 = edge->p1 + (edge->p2 - edge->p1) * (-d1 / per);
        }
    }
    else if (d2 < 0)
    {
        if (removeClippedPoint)
        {
            edge->p2 = edge->p1;
            edge->id2 = edge->id1;
        }
        else
        {
            edge->p2 = edge->p2 + (edge->p1 - edge->p2) * (-d2 / per);
        }
    }
}

static void find_contact_points(const glm::vec2& n, RigidBody* a, RigidBody* b, ContactManifold* out)
{
    Edge edgeA = find_farthest_edge(a, n);
    Edge edgeB = find_farthest_edge(b, -n);

    Edge* ref = &edgeB; // Reference edge
    Edge* inc = &edgeA; // Incidence edge
    out->bodyA = a;
    out->bodyB = b;
    out->contactNormal = n;
    out->featureFlipped = false;

    float aPerpendicularness = glm::abs(glm::dot(edgeA.dir, n));
    float bPerpendicularness = glm::abs(glm::dot(edgeB.dir, n));

    if (aPerpendicularness <= bPerpendicularness)
    {
        ref = &edgeA;
        inc = &edgeB;
        out->bodyA = b;
        out->bodyB = a;
        out->contactNormal = -n;
        out->featureFlipped = true;
    }

    clip_edge(inc, ref->p1, ref->dir);
    clip_edge(inc, ref->p2, -ref->dir);
    clip_edge(inc, ref->p1, out->contactNormal, true);

    // If two points are closer than threshold, merge them into one point
    if (inc->Length() <= CONTACT_MERGE_THRESHOLD)
    {
        out->contactPoints[0] = { inc->p1, inc->id1 };
        out->numContacts = 1;
    }
    else
    {
        out->contactPoints[0] = { inc->p1, inc->id1 };
        out->contactPoints[1] = { inc->p2, inc->id2 };
        out->numContacts = 2;
    }
}

static bool circle_vs_circle(Circle* a, Circle* b, ContactManifold* out)
{
    float d = glm::distance2(a->position, b->position);
    const float r2 = a->GetRadius() + b->GetRadius();

    if (d > r2 * r2)
    {
        return false;
    }
    else
    {
        if (out == nullptr) return true;

        d = glm::sqrt(d);

        out->bodyA = a;
        out->bodyB = b;
        out->contactNormal = glm::normalize(b->position - a->position);
        out->contactPoints[0] = { a->position + (out->contactNormal * a->GetRadius()), -1 };
        out->numContacts = 1;
        out->penetrationDepth = r2 - d;
        out->featureFlipped = false;

        // Apply axis weight to improve coherence
        if (APPLY_AXIS_WEIGHT && glm::dot(out->contactNormal, weightAxis) < 0.0f)
        {
            RigidBody* tmp = out->bodyA;
            out->bodyA = out->bodyB;
            out->bodyB = tmp;
            out->contactNormal *= -1;
            out->featureFlipped = out->bodyA != a;
        }
        out->contactTangent = glm::vec2(-out->contactNormal.y, out->contactNormal.x);

        return true;
    }
}

static bool convex_vs_convex(RigidBody* a, RigidBody* b, ContactManifold* out)
{
    GJKResult gjkResult = gjk(a, b);

    if (!gjkResult.collide)
    {
        return false;
    }
    else
    {
        if (out == nullptr) return true;

        // If the gjk termination simplex has vertices less than 3, expand to full simplex
        // Because EPA needs a full n-simplex to get started
        Simplex& simplex = gjkResult.simplex;
        switch (simplex.Count())
        {
        case 1:
        {
            const glm::vec2& v = simplex.vertices[0];
            glm::vec2 randomSupport = cso_support(a, b, glm::vec2{ 1.0f, 0.0f });

            if (randomSupport == v)
            {
                randomSupport = cso_support(a, b, glm::vec2{ -1.0f, 0.0f });
            }

            simplex.AddVertex(randomSupport);
        }
        case 2:
        {
            Edge e{ simplex.vertices[0], simplex.vertices[1] };
            glm::vec2 normalSupport = cso_support(a, b, e.Normal());

            if (simplex.ContainsVertex(normalSupport))
            {
                simplex.AddVertex(cso_support(a, b, -e.Normal()));
            }
            else
            {
                simplex.AddVertex(normalSupport);
            }
        }
        }

        EPAResult epaResult = epa(a, b, gjkResult.simplex);

        find_contact_points(epaResult.contactNormal, a, b, out);

        // Apply axis weight to improve coherence
        if (APPLY_AXIS_WEIGHT && glm::dot(out->contactNormal, weightAxis) < 0.0f)
        {
            RigidBody* tmp = out->bodyA;
            out->bodyA = out->bodyB;
            out->bodyB = tmp;
            out->contactNormal *= -1;
            out->featureFlipped = out->bodyA != a;
        }
        out->contactTangent = glm::vec2(-out->contactNormal.y, out->contactNormal.x);
        out->penetrationDepth = epaResult.penetrationDepth;

        return true;
    }
}

bool detect_collision(RigidBody* a, RigidBody* b, ContactManifold* out)
{
    out->numContacts = 0;
    out->penetrationDepth = 0.0f;

    // Circle vs. Circle collision
    if (a->GetShape() == RigidBody::Shape::ShapeCircle && b->GetShape() == RigidBody::Shape::ShapeCircle)
    {
        return circle_vs_circle(static_cast<Circle*>(a), static_cast<Circle*>(b), out);
    }
    else
    {
        return convex_vs_convex(a, b, out);
    }
}

bool test_point_inside(RigidBody* b, const glm::vec2& p)
{
    glm::vec2 localP = b->GlobalToLocal() * p;

    if (b->GetShape() == RigidBody::Shape::ShapeCircle)
    {
        return glm::length(localP) <= static_cast<Circle*>(b)->GetRadius();
    }
    else if (b->GetShape() == RigidBody::Shape::ShapePolygon)
    {
        Polygon* p = static_cast<Polygon*>(b);
        const std::vector<glm::vec2>& vertices = p->GetVertices();

        float dir = glm::cross(vertices[0] - localP, vertices[1] - localP);

        for (uint32_t i = 1; i < vertices.size(); i++)
        {
            float nDir = glm::cross(vertices[i] - localP, vertices[(i + 1) % vertices.size()] - localP);

            if (dir * nDir < 0) return false;
        }

        return true;
    }
    else
    {
        throw std::exception("Not a supported shape");
    }
}

float compute_distance(RigidBody* a, RigidBody* b)
{
    GJKResult gr = gjk(a, b, false);

    if (gr.collide)
    {
        return 0.0f;
    }
    else
    {
        ClosestResult cr = gr.simplex.GetClosest({ 0.0f, 0.0f });

        return glm::length(cr.point);
    }
}

float compute_distance(RigidBody* b, const glm::vec2& p)
{
    return glm::distance(get_closest_point(b, p), p);
}

glm::vec2 get_closest_point(RigidBody* b, const glm::vec2& p)
{
    glm::vec2 localP = b->GlobalToLocal() * p;

    if (test_point_inside(b, localP))
    {
        return p;
    }
    else
    {
        glm::vec2 dir = glm::normalize(localP - b->position);

        if (b->GetType() == RigidBody::Shape::ShapeCircle)
        {
            glm::vec2 localR = b->position + dir * static_cast<Circle*>(b)->GetRadius();
            return b->LocalToGlobal() * localR;
        }
        else if (b->GetType() == RigidBody::Shape::ShapePolygon)
        {
            Polygon* poly = static_cast<Polygon*>(b);
            const std::vector<glm::vec2>& v = poly->GetVertices();

            SupportResult sr = support(poly, dir);
            Simplex s;

            s.AddVertex(v[sr.index]);
            s.AddVertex(v[(sr.index + 1) % v.size()]);
            s.AddVertex(v[(sr.index + 2) % v.size()]);

            ClosestResult cr = s.GetClosest(localP);
            return b->LocalToGlobal() * cr.point;
        }
        else
        {
            throw std::exception("Not a supported shape");
        }
    }
}

} // namespace spe
