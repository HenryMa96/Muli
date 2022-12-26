#include "demo.h"
#include "game.h"
#include "window.h"

namespace muli
{

class ShapeCastTest : public Demo
{
public:
    bool hit;
    ShapeCastOutput output;

    Vec2 translationA{ -2.0f, 2.0f };
    Vec2 translationB{ 5.0f, 0.0f };

    ShapeCastTest(Game& game)
        : Demo(game)
    {
        options.show_contact_normal = true;
        options.show_contact_point = true;
        settings.apply_gravity = false;
        settings.sleeping = false;

        RigidBody* b = world->CreateCapsule(1.0f, 0.3f);
        b->SetPosition(4.0f, 3.0f);

        b = world->CreateBox(0.5f);
        b->SetPosition(0.0f, 5.0f);
    }

    void Render() override
    {
        if (world->GetBodyCount() > 1)
        {
            RigidBody* a = world->GetBodyList();
            RigidBody* b = a->GetNext();

            Collider* ca = a->GetColliderList();
            Collider* cb = b->GetColliderList();

            const Shape* sa = ca->GetShape();
            const Shape* sb = cb->GetShape();

            const Transform& tfA = a->GetTransform();
            const Transform& tfB = b->GetTransform();

            Transform thA = tfA;
            Transform thB = tfB;

            const RigidBodyRenderer& rr = game.GetRigidBodyRenderer();
            std::vector<Vec2>& pl = game.GetPointList();
            std::vector<Vec2>& ll = game.GetLineList();

            hit = ShapeCast(sa, tfA, sb, tfB, translationA, translationB, &output);
            if (hit)
            {
                thA.position += translationA * output.t;
                rr.Render(ca, thA);
                thB.position += translationB * output.t;
                rr.Render(cb, thB);

                pl.push_back(output.point);
                ll.push_back(output.point);
                ll.push_back(output.point + output.normal * 0.2f);
            }
            else
            {
                thA.position += translationA;
                rr.Render(ca, thA);
                thB.position += translationB;
                rr.Render(cb, thB);
            }

            ll.push_back(tfA.position);
            ll.push_back(thA.position);
            ll.push_back(tfB.position);
            ll.push_back(thB.position);
        }
    }

    void UpdateUI() override
    {
        ImGui::SetNextWindowPos({ Window::Get().GetWindowSize().x - 5, 5 }, ImGuiCond_Once, { 1.0f, 0.0f });
        ImGui::SetNextWindowSize({ 360, 95 }, ImGuiCond_Once);

        if (ImGui::Begin("Shape cast"))
        {
            if (world->GetBodyCount() > 1)
            {
                ImGui::DragFloat2("Translation A", &translationA.x, 0.1f);
                ImGui::DragFloat2("Translation B", &translationB.x, 0.1f);
            }
            if (hit)
            {
                ImGui::Text("Hit! at t: %.4f", output.t);
            }
        }
        ImGui::End();
    }

    ~ShapeCastTest()
    {
        options.show_contact_normal = false;
        options.show_contact_point = false;
    }

    static Demo* Create(Game& game)
    {
        return new ShapeCastTest(game);
    }
};

DemoFrame shape_cast{ "Shape cast", ShapeCastTest::Create };

} // namespace muli