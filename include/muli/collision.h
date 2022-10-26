#pragma once

#include "common.h"
#include "contact_point.h"
#include "edge.h"
#include "settings.h"

/*
 *           \        /         ↑
 *            \      /          | <- Contact normal
 *    ---------\----/-------------------------------  <- Reference edge
 *              \  /
 *               \/  <- Incident point(Contact point)
 *
 */

namespace muli
{

class RigidBody;
class Circle;
class Capsule;
class Polygon;

// 64byte
struct ContactManifold
{
    ContactPoint contactPoints[MAX_CONTACT_POINT];
    ContactPoint referencePoint;
    Vec2 contactNormal; // Contact normal is always pointing from a to b
    Vec2 contactTangent;
    float penetrationDepth;
    uint32 numContacts;
    bool featureFlipped;
};

// Define a ray such that:
// Ray = from + maxFraction * (to - from)
struct RayCastInput
{
    Vec2 from;
    Vec2 to;
    float maxFraction;
};

struct RayCastOutput
{
    Vec2 normal;
    float fraction;
};

typedef bool DetectionFunction(RigidBody*, RigidBody*, ContactManifold*);
typedef float DistanceFunction(RigidBody*, RigidBody*);

bool DetectCollision(RigidBody* a, RigidBody* b, ContactManifold* manifold = nullptr);

float ComputeDistance(RigidBody* a, RigidBody* b);
float ComputeDistance(RigidBody* b, Vec2 q);

Vec2 GetClosestPoint(Circle* b, Vec2 q);
Vec2 GetClosestPoint(Capsule* b, Vec2 q);
Vec2 GetClosestPoint(Polygon* b, Vec2 q);
Vec2 GetClosestPoint(RigidBody* b, Vec2 q);

Edge GetIntersectingEdge(Polygon* p, Vec2 dir);
bool SAT(Polygon* a, Polygon* b);

} // namespace muli