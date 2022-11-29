#pragma once

#include "broad_phase.h"
#include "contact.h"

namespace muli
{
class World;
extern void InitializeDetectionFunctionMap();

class ContactManager
{
public:
    ContactManager(World& world);
    ~ContactManager();

    void Step(float dt);
    void Add(Collider* collider);
    void Remove(Collider* collider);
    void Reset();
    uint32 GetContactCount() const;

private:
    friend class World;
    friend class RigidBody;

    World& world;
    BroadPhase broadPhase;
    Contact* contactList = nullptr;
    uint32 contactCount = 0;

    void Destroy(Contact* c);
};

inline ContactManager::ContactManager(World& _world)
    : world{ _world }
    , broadPhase{ _world }
{
    InitializeDetectionFunctionMap();
}

inline ContactManager::~ContactManager()
{
    Reset();
}

inline void ContactManager::Add(Collider* collider)
{
    broadPhase.Add(collider);
}

inline void ContactManager::Remove(Collider* collider)
{
    broadPhase.Remove(collider);

    RigidBody* body = collider->body;

    // Destroy any contacts associated with the collider
    ContactEdge* edge = body->contactList;
    while (edge)
    {
        Contact* contact = edge->contact;
        edge = edge->next;

        Collider* colliderA = contact->GetColliderA();
        Collider* colliderB = contact->GetColliderB();

        if (collider == colliderA || collider == colliderB)
        {
            Destroy(contact);

            colliderA->body->Awake();
            colliderB->body->Awake();
        }
    }
}

inline uint32 ContactManager::GetContactCount() const
{
    return contactCount;
}

} // namespace muli
