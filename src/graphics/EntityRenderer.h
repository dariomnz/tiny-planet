#pragma once

#include <GLES3/gl3.h>

#include <cstddef>
#include <glm/glm.hpp>
#include <vector>

#include "gl/Program.h"
#include "gl/Types.h"
#include "gl/UniformBuffer.h"
#include "gl/VertexArray.h"
#include "gl/VertexBuffer.h"

struct Projectile;
struct EnemyBullet;

// Batched cube/triangle renderer (Maplex Renderer3D-style).
// Usage per frame: begin() -> draw*() (append only) -> end() (flushes).
// Batches flush mid-frame on overflow (NextBatch pattern), so any count
// renders correctly at the cost of extra draws; caps only size the
// per-flushUploads (10x pools fit in one flush).
class EntityRenderer {
   public:
    static constexpr uint32_t kMaxInstances = 12288;
    static constexpr uint32_t kMaxEnemyInstances = 4096;
    static constexpr uint32_t kMaxTriVerts = 8192;

    EntityRenderer();

    // Sets Frame UBO (projView/playerPos/curveK) and clears batches.
    void begin(const glm::mat4 &projView, const glm::vec2 &playerPos, float curveK);
    void drawPlayer(const glm::mat4 &model, const glm::vec3 &color);
    void drawNose(const glm::mat4 &model, const glm::vec3 &color);
    void drawProjectiles(const Projectile *items, std::size_t count, const glm::vec3 &color);
    void drawEnemies(const glm::vec2 *positions, const float *scales, const glm::vec3 *colors,
                     std::size_t count);
    void drawEnemyBullets(const EnemyBullet *items, std::size_t count, const glm::vec3 &color);
    void drawGems(const glm::vec2 *positions, std::size_t count, const glm::vec3 &color);
    // Flushes any remainder: TriBatch + small-cube instances + enemies.
    void end();

   private:
    void submitMesh(const std::vector<VertexPN> &mesh, const glm::mat4 &model, const glm::vec3 &color);
    void pushInstance(const glm::vec3 &offset, float scale, const glm::vec3 &color);
    void pushEnemyInstance(const glm::vec3 &offset, float scale, const glm::vec3 &color);
    void flushTris();
    void flushInstances();
    void flushEnemies();

    Program m_instProg;
    Program m_triProg;
    UniformBuffer m_frameUbo;

    VertexArray m_cubeVao;
    VertexBuffer m_baseVbo;
    VertexBuffer m_instanceVbo;

    VertexArray m_enemyVao;
    VertexBuffer m_enemyBaseVbo;
    VertexBuffer m_enemyInstanceVbo;

    VertexArray m_triVao;
    VertexBuffer m_triVbo;

    std::vector<VertexPN> m_playerMesh;
    std::vector<VertexPN> m_noseMesh;

    std::vector<InstanceData> m_instances;
    std::vector<InstanceData> m_enemyInstances;
    std::vector<TriVertex> m_tris;
};
