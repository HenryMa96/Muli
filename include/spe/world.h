#pragma once

#include "common.h"
#include "aabbtree.h"
#include "detection.h"
#include "rigidbody.h"
#include "contact_constraint.h"
#include "grab_joint.h"

namespace spe
{
// Simulation settings
struct Settings
{
    float DT = 1.0f / 60.0f;
    float INV_DT = 60.0f;

    bool APPLY_GRAVITY = true;
    glm::vec2 GRAVITY = glm::vec2{ 0.0f, -10.0f };

    bool IMPULSE_ACCUMULATION = true;
    bool WARM_STARTING = true;
    bool APPLY_WARM_STARTING_THRESHOLD = true;
    float WARM_STARTING_THRESHOLD = 0.005f * 0.005f - glm::epsilon<float>();

    bool POSITION_CORRECTION = true;
    float POSITION_CORRECTION_BETA = 0.2f;

    float PENETRATION_SLOP = 0.005f;
    float RESTITUTION_SLOP = 0.5f;

    bool BLOCK_SOLVE = true;
    int32_t SOLVE_ITERATION = 10;

    float REST_LINEAR_TOLERANCE = 0.01f * 0.01f;
    float REST_ANGULAR_TOLERANCE = (0.5f * glm::pi<float>() / 180.0f) * (0.5f * glm::pi<float>() / 180.0f);

    bool SLEEPING_ENABLED = true;
    float SLEEPING_TRESHOLD = 0.5f;

    AABB VALID_REGION{ glm::vec2{-FLT_MAX, -10.0f},glm::vec2{FLT_MAX, FLT_MAX} };
};

class World final
{
    friend class Island;

public:
    World(const Settings& simulationSettings);
    ~World() noexcept;

    World(const World&) noexcept = delete;
    World& operator=(const World&) noexcept = delete;

    World(World&&) noexcept = delete;
    World& operator=(World&&) noexcept = delete;

    void Update(float dt);
    void Reset();

    void Add(RigidBody* body);
    void Add(const std::vector<RigidBody*>& bodies);
    void Remove(RigidBody* body);
    void Remove(const std::vector<RigidBody*>& bodies);
    void Remove(Joint* joint);
    void Remove(const std::vector<Joint*>& joints);

    Box* CreateBox(float size, BodyType type = Dynamic, float density = DEFAULT_DENSITY);
    Box* CreateBox(float width, float height, BodyType type = Dynamic, float density = DEFAULT_DENSITY);
    Circle* CreateCircle(float radius, BodyType type = Dynamic, float density = DEFAULT_DENSITY);
    Polygon* CreatePolygon(std::vector<glm::vec2> vertices, BodyType type = Dynamic, bool resetPosition = true, float density = DEFAULT_DENSITY);

    GrabJoint* CreateGrabJoint(RigidBody* body, glm::vec2 anchor, glm::vec2 target, float frequency = 0.8f, float dampingRatio = 0.6f, float jointMass = -1.0f);

    std::vector<RigidBody*> QueryPoint(const glm::vec2& point) const;
    std::vector<RigidBody*> QueryRegion(const AABB& region) const;

    const std::vector<RigidBody*>& GetBodies() const;
    const size_t GetSleepingBodyCount() const;
    const size_t GetSleepingIslandCount() const;
    const AABBTree& GetBVH() const;
    const std::vector<ContactConstraint>& GetContactConstraints() const;

    const std::vector<Joint*>& GetJoints() const;

    void AddPassTestPair(RigidBody* bodyA, RigidBody* bodyB);
    void RemovePassTestPair(RigidBody* bodyA, RigidBody* bodyB);

private:
    const Settings& settings;
    uint32_t uid{ 0 };

    // Dynamic AABB Tree for broad phase collision detection
    AABBTree tree{};
    // All registered rigid bodies
    std::vector<RigidBody*> bodies{};
    std::vector<std::pair<RigidBody*, RigidBody*>> pairs{};

    std::unordered_set<uint32_t> passTestSet{};

    // Constraints to be solved
    std::vector<ContactConstraint> contactConstraints{};
    std::unordered_map<uint32_t, ContactConstraint*> contactConstraintMap{};
    std::vector<ContactConstraint> newContactConstraints{};
    std::unordered_map<uint32_t, ContactConstraint*> newContactConstraintMap{};

    std::vector<Joint*> joints{};
    std::unordered_map<uint32_t, Joint*> jointMap{};

    bool forceIntegration = false;
    uint32_t numIslands = 0;
    size_t sleepingIslands = 0;
    size_t sleepingBodies = 0;
};
}