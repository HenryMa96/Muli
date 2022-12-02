#include "muli/contact.h"
#include "muli/block_solver.h"
#include "muli/callbacks.h"
#include "muli/contact_solver.h"
#include "muli/world.h"

namespace muli
{

extern DetectionFunction* DetectionFunctionMap[Shape::Type::shape_count][Shape::Type::shape_count];

Contact::Contact(Collider* _colliderA, Collider* _colliderB, const WorldSettings& _settings)
    : Constraint(_colliderA->body, _colliderB->body, _settings)
    , colliderA{ _colliderA }
    , colliderB{ _colliderB }
    , touching{ false }
{
    muliAssert(colliderA->GetType() >= colliderB->GetType());

    manifold.numContacts = 0;

    beta = settings.POSITION_CORRECTION_BETA;
    restitution = MixRestitution(colliderA->GetRestitution(), colliderB->GetRestitution());
    friction = MixFriction(colliderA->GetFriction(), colliderB->GetFriction());
    surfaceSpeed = colliderB->GetSurfaceSpeed() - colliderA->GetSurfaceSpeed();

    collisionDetectionFunction = DetectionFunctionMap[colliderA->GetType()][colliderB->GetType()];
    muliAssert(collisionDetectionFunction != nullptr);
}

void Contact::Update()
{
    ContactManifold oldManifold = manifold;
    float oldNormalImpulse[MAX_CONTACT_POINT];
    float oldTangentImpulse[MAX_CONTACT_POINT];

    bool wasTouching = touching;

    // clang-format off
    touching = collisionDetectionFunction(colliderA->shape, bodyA->transform,
                                          colliderB->shape, bodyB->transform,
                                          &manifold);
    // clang-format on

    for (uint32 i = 0; i < MAX_CONTACT_POINT; ++i)
    {
        oldNormalImpulse[i] = normalSolvers[i].impulseSum;
        oldTangentImpulse[i] = tangentSolvers[i].impulseSum;
        normalSolvers[i].impulseSum = 0.0f;
        tangentSolvers[i].impulseSum = 0.0f;
    }

    if (touching == false)
    {
        if (wasTouching == true)
        {
            if (colliderA->ContactListener) colliderA->ContactListener->OnContactEnd(colliderA, colliderB, this);
            if (colliderB->ContactListener) colliderB->ContactListener->OnContactEnd(colliderB, colliderA, this);
        }

        return;
    }

    if (wasTouching == false && touching == true)
    {
        if (colliderA->ContactListener) colliderA->ContactListener->OnContactBegin(colliderA, colliderB, this);
        if (colliderB->ContactListener) colliderB->ContactListener->OnContactBegin(colliderB, colliderA, this);
    }

    if (wasTouching == true && touching == true)
    {
        if (colliderA->ContactListener) colliderA->ContactListener->OnContactTouching(colliderA, colliderB, this);
        if (colliderB->ContactListener) colliderB->ContactListener->OnContactTouching(colliderB, colliderA, this);
    }

    if (manifold.featureFlipped)
    {
        b1 = bodyB;
        b2 = bodyA;
    }
    else
    {
        b1 = bodyA;
        b2 = bodyB;
    }

    // Warm start the contact solver
    for (uint32 n = 0; n < manifold.numContacts; ++n)
    {
        uint32 o = 0;
        for (; o < oldManifold.numContacts; ++o)
        {
            if (manifold.contactPoints[n].id == oldManifold.contactPoints[o].id)
            {
                break;
            }
        }

        if (o < oldManifold.numContacts)
        {
            normalSolvers[n].impulseSum = oldNormalImpulse[o];
            tangentSolvers[n].impulseSum = oldTangentImpulse[o];
        }
    }
}

void Contact::Prepare()
{
    for (uint32 i = 0; i < manifold.numContacts; ++i)
    {
        normalSolvers[i].Prepare(this, i, manifold.contactNormal, ContactSolver::Type::normal);
        tangentSolvers[i].Prepare(this, i, manifold.contactTangent, ContactSolver::Type::tangent);
        positionSolvers[i].Prepare(this, i);
    }

    if (manifold.numContacts == 2 && settings.BLOCK_SOLVE)
    {
        blockSolver.Prepare(this);
    }
}

void Contact::SolveVelocityConstraint()
{
    // Solve tangential constraint first
    for (uint32 i = 0; i < manifold.numContacts; ++i)
    {
        tangentSolvers[i].Solve(&normalSolvers[i]);
    }

    if (manifold.numContacts == 1 || !settings.BLOCK_SOLVE || (blockSolver.enabled == false))
    {
        for (uint32 i = 0; i < manifold.numContacts; ++i)
        {
            normalSolvers[i].Solve();
        }
    }
    else // Solve two contact constraint in one shot using block solver
    {
        blockSolver.Solve();
    }
}

bool Contact::SolvePositionConstraint()
{
    bool solved = true;

    cLinearImpulseA.SetZero();
    cLinearImpulseB.SetZero();
    cAngularImpulseA = 0.0f;
    cAngularImpulseB = 0.0f;

    // Solve position constraint
    for (uint32 i = 0; i < manifold.numContacts; ++i)
    {
        solved &= positionSolvers[i].Solve();
    }

    b1->position += b1->invMass * cLinearImpulseA;
    b1->angle += b1->invInertia * cAngularImpulseA;
    b2->position += b2->invMass * cLinearImpulseB;
    b2->angle += b2->invInertia * cAngularImpulseB;

    return solved;
}

} // namespace muli
