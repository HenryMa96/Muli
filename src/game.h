#pragma once

#include "common.h"
#include "entity.h"
#include "rendering/shader.h"
#include "rendering/myshader.h"
#include "rendering/mesh.h"
#include "physics/polygon.h"
#include "physics/circle.h"
#include "physics/box.h"
#include "physics/aabbtree.h"
#include "input.h"

namespace spe
{
    class Application;

    class Game final
    {
    public:
        Game(Application& app);
        ~Game() noexcept = default;

        Game(const Game&) noexcept = delete;
        Game(Game&&) noexcept = delete;
        Game& operator=(const Game&) noexcept = delete;
        Game& operator=(Game&&) noexcept = delete;

        void Update(float dt);
        void Render();

        void UpdateProjectionMatrix();

    private:
        float zoom = 100.0f;

        Application& app;
        std::unique_ptr<MyShader> s;
        std::unique_ptr<Mesh> m;
        float time{ 0.0f };
    };
}