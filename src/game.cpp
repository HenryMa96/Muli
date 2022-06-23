#include "game.h"
#include "engine.h"

using namespace spe;

Game::Game(Engine& _engine) :
    engine{ _engine }
{
    s = MyShader::Create();
    s->Use();

    viewportSize = engine.GetWindowSize();
    glm::vec2 windowSize = viewportSize;
    windowSize /= 100.0f;

    s->SetProjectionMatrix(glm::ortho(-windowSize.x, windowSize.x, -windowSize.y, windowSize.y, 0.0f, 100.0f));
    s->SetViewMatrix(glm::translate(glm::mat4{ 1.0 }, glm::vec3(0, 0, -1)));

    m = std::unique_ptr<Mesh>(new Mesh(
        {
            glm::vec3(0.5f,  0.5f, 0.0f),
            glm::vec3(0.5f, -0.5f, 0.0f),
            glm::vec3(-0.5f, -0.5f, 0.0f),
            glm::vec3(-0.5f,  0.5f, 0.0f),
        }
        ,
        {
            glm::vec2{1.0f, 1.0f},
            glm::vec2{1.0f, 0.0f},
            glm::vec2{0.0f, 0.0f},
            glm::vec2{0.0f, 1.0f},
        }
        ,
        {
            0, 1, 3,
            1, 2, 3,
        }
        ));

    auto p = Polygon
    (
        {
            glm::vec2(0.5f,  0.5f),
            glm::vec2(0.5f, -0.5f),
            glm::vec2(-0.5f, -0.5f),
            glm::vec2(-0.5f,  0.5f),
        }
    );

    SPDLOG_INFO("{} {} {} {}", p.GetMass(), p.GetInverseMass(), p.GetInertia(), p.GetInverseInertia());

    auto c = Circle(1);

    SPDLOG_INFO("{} {} {} {}", c.GetMass(), c.GetInverseMass(), c.GetInertia(), c.GetInverseInertia());

    auto b = Box(1, 1);

    SPDLOG_INFO("{} {} {} {}", b.GetMass(), b.GetInverseMass(), b.GetInertia(), b.GetInverseInertia());

    auto at = AABBTree();

    auto k = at.Add(c);
    at.Add(b);

    for(int i = 0; i < 10; i++)
    {
        at.Add(Box(1, 1));
    }

    SPDLOG_INFO("manual remove");
    at.Remove(k);

    SPDLOG_INFO("collision pairs {}", at.GetCollisionPairs().size());
    SPDLOG_INFO("Tree cost {}", at.GetTreeCost());

    SPDLOG_INFO("--------");
}

void Game::Update(float dt)
{
    time += dt;

    // Update projection matrix
    glm::vec2 windowSize = engine.GetWindowSize();
    if (viewportSize != windowSize)
    {
        UpdateProjectionMatrix();
    }

    if (ImGui::Begin("Control Panel"))
    {
        // ImGui::Text("This is some useful text.");

        static int f = 60;
        if (ImGui::SliderInt("Frame rate", &f, 30, 300))
        {
            engine.SetFrameRate(f);
        }
        ImGui::Separator();

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::Separator();

        ImGui::ColorEdit4("Background color", glm::value_ptr(engine.clearColor));

        ImGui::Separator();

        if (ImGui::SliderFloat("Zoom", &zoom, 10, 500))
        {
            UpdateProjectionMatrix();
        }
    }
    ImGui::End();
}

void Game::Render()
{
    s->Use();
    {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        glm::mat4 r = glm::rotate(glm::mat4{ 1.0f }, glm::radians(time * 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        s->SetModelMatrix(t * r);
        s->SetColor({ glm::sin(time * 2) * 0.5f + 1.0f, glm::cos(time * 3) * 0.5f + 1.0f, glm::sin(time * 1.5) * 0.5f + 1.0f });

        m->Draw();
    }
}

void Game::UpdateProjectionMatrix()
{
    glm::vec2 windowSize = engine.GetWindowSize();
    viewportSize = windowSize;
    windowSize /= zoom;

    s->SetProjectionMatrix(glm::ortho(-windowSize.x, windowSize.x, -windowSize.y, windowSize.y, 0.0f, 100.0f));
}

Game::~Game()
{

}