#include "demo.h"
#include "game.h"

namespace muli
{

class MotorJointTest : public Demo
{
public:
    MotorJoint* motor;
    RigidBody* windmill;

    MotorJointTest(Game& game)
        : Demo(game)
    {
        RigidBody* stick = world->CreateCapsule(Vec2{ 0.0f, 0.0f }, Vec2{ 0.0f, 3.0f }, 0.075f, false, RigidBody::Type::Static);

        windmill = world->CreateCapsule(2.0f, 0.075f, true);
        windmill->SetPosition(0.0f, 3.0f);

        CollisionFilter filter;
        filter.filter = 1 << 1;
        filter.mask = ~(1 << 1);

        stick->SetCollisionFilter(filter);
        windmill->SetCollisionFilter(filter);

        motor = world->CreateMotorJoint(stick, windmill, windmill->GetPosition(), 1000.0f, 100.0f);
    }

    float t = 0.0f;

    void Step() override
    {
        Demo::Step();
        motor->SetAngularOffset(windmill->GetAngle() + 5.0f * dt);

        if (game.GetTime() > t + 0.2f)
        {
            RigidBody* c = world->CreateRegularPolygon(0.18f, LinearRand(3, 8));
            c->SetPosition(LinearRand(Vec2{ -2.0f, 6.0f }, Vec2{ 2.0f, 6.0f }));
            game.RegisterRenderBody(c);

            t = game.GetTime();
        }
    }

    static Demo* Create(Game& game)
    {
        return new MotorJointTest(game);
    }
};

DemoFrame windmill{ "Motor joint test", MotorJointTest::Create };

} // namespace muli
