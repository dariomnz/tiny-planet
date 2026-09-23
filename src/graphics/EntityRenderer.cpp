#include "graphics/EntityRenderer.h"

#include "Config.h"
#include "gl/Layout.h"
#include "gl/RenderCommand.h"
#include "graphics/Meshes.h"
#include "shaders/Shaders.h"
#include "world/EnemyBullets.h"
#include "world/Projectiles.h"

namespace {
void bindFrameBlock(const Program &prog, uint32_t binding = 0) {
    const GLuint idx = glGetUniformBlockIndex(prog.id(), "Frame");
    if (idx != GL_INVALID_INDEX) glUniformBlockBinding(prog.id(), idx, binding);
}
}  // namespace

EntityRenderer::EntityRenderer()
    : m_instProg(shaders::kEntityInstVert, shaders::kEntityInstFrag),
      m_triProg(shaders::kEntityTriVert, shaders::kEntityInstFrag),
      m_frameUbo(sizeof(FrameData), 0),
      m_baseVbo(nullptr, 0),  // replaced below via move-assign
      m_instanceVbo(kMaxInstances * sizeof(InstanceData)),
      m_enemyBaseVbo(nullptr, 0),  // replaced below via move-assign
      m_enemyInstanceVbo(kMaxEnemyInstances * sizeof(InstanceData)),
      m_triVbo(kMaxTriVerts * sizeof(TriVertex)) {
    bindFrameBlock(m_instProg, 0);
    bindFrameBlock(m_triProg, 0);

    m_playerMesh = meshes::makePlayerCube();
    m_noseMesh = meshes::makeNose();
    const auto projMesh = meshes::makeProjectileCube(config::kProjSize);

    // Small-cube batch (divisor 0): centered 0.25 mesh for
    // projectiles/bullets/gems.
    m_baseVbo = VertexBuffer(projMesh.data(), static_cast<uint32_t>(projMesh.size() * sizeof(VertexPN)));
    m_baseVbo.setLayout({{ShaderDataType::Float3, "inPos"}, {ShaderDataType::Float3, "inNormal"}});

    m_instanceVbo.setLayout({{ShaderDataType::Float3, "inOffset", 1},
                             {ShaderDataType::Float, "inScale", 1},
                             {ShaderDataType::Float3, "inColor", 1}});

    m_cubeVao.addVertexBuffer(m_baseVbo);
    m_cubeVao.addVertexBuffer(m_instanceVbo);
    VertexArray::unbind();

    // Enemy batch: same player-sized mesh the old per-draw path used
    // (x/y +/-0.5, z 0..2), so scale/offset semantics are unchanged.
    m_enemyBaseVbo =
        VertexBuffer(m_playerMesh.data(), static_cast<uint32_t>(m_playerMesh.size() * sizeof(VertexPN)));
    m_enemyBaseVbo.setLayout({{ShaderDataType::Float3, "inPos"}, {ShaderDataType::Float3, "inNormal"}});
    m_enemyInstanceVbo.setLayout({{ShaderDataType::Float3, "inOffset", 1},
                                  {ShaderDataType::Float, "inScale", 1},
                                  {ShaderDataType::Float3, "inColor", 1}});
    m_enemyVao.addVertexBuffer(m_enemyBaseVbo);
    m_enemyVao.addVertexBuffer(m_enemyInstanceVbo);
    VertexArray::unbind();

    m_triVbo.setLayout({{ShaderDataType::Float3, "inPos"},
                        {ShaderDataType::Float3, "inNormal"},
                        {ShaderDataType::Float3, "inColor"}});
    m_triVao.addVertexBuffer(m_triVbo);
    VertexArray::unbind();

    m_instances.reserve(kMaxInstances);
    m_enemyInstances.reserve(kMaxEnemyInstances);
    m_tris.reserve(kMaxTriVerts);
}

void EntityRenderer::begin(const glm::mat4 &projView, const glm::vec2 &playerPos, float curveK) {
    FrameData frame;
    frame.projView = projView;
    frame.playerPos = playerPos;
    frame.curveK = curveK;
    m_frameUbo.setData(&frame, sizeof(frame));
    m_instances.clear();
    m_enemyInstances.clear();
    m_tris.clear();
}

void EntityRenderer::submitMesh(const std::vector<VertexPN> &mesh, const glm::mat4 &model,
                                const glm::vec3 &color) {
    if (mesh.size() > kMaxTriVerts) return;  // single mesh bigger than a batch: skip (never happens)
    if (m_tris.size() + mesh.size() > kMaxTriVerts) {
        flushTris();  // NextBatch: draw what's queued, keep accepting
        m_tris.clear();
    }
    const glm::mat3 nrm(model);
    for (const auto &v : mesh) {
        const glm::vec4 w = model * glm::vec4(v.pos, 1.0f);
        m_tris.push_back({glm::vec3(w), nrm * v.normal, color});
    }
}

void EntityRenderer::pushInstance(const glm::vec3 &offset, float scale, const glm::vec3 &color) {
    if (m_instances.size() >= kMaxInstances) {
        flushInstances();  // NextBatch: draw what's queued, keep accepting
        m_instances.clear();
    }
    m_instances.push_back({offset, scale, color});
}

void EntityRenderer::pushEnemyInstance(const glm::vec3 &offset, float scale, const glm::vec3 &color) {
    if (m_enemyInstances.size() >= kMaxEnemyInstances) {
        flushEnemies();  // NextBatch: draw what's queued, keep accepting
        m_enemyInstances.clear();
    }
    m_enemyInstances.push_back({offset, scale, color});
}

void EntityRenderer::drawPlayer(const glm::mat4 &model, const glm::vec3 &color) {
    submitMesh(m_playerMesh, model, color);
}

void EntityRenderer::drawNose(const glm::mat4 &model, const glm::vec3 &color) {
    submitMesh(m_noseMesh, model, color);
}

void EntityRenderer::drawProjectiles(const Projectile *items, std::size_t count, const glm::vec3 &color) {
    for (std::size_t i = 0; i < count; ++i) pushInstance(items[i].pos, 1.0f, color);
}

void EntityRenderer::drawEnemies(const glm::vec2 *positions, const float *scales, const glm::vec3 *colors,
                                 std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        const float s = scales ? scales[i] : 1.0f;
        const glm::vec3 c = colors ? colors[i] : glm::vec3(1.0f, 0.15f, 0.15f);
        pushEnemyInstance(glm::vec3(positions[i].x, positions[i].y, 0.0f), s, c);
    }
}

void EntityRenderer::drawEnemyBullets(const EnemyBullet *items, std::size_t count, const glm::vec3 &color) {
    for (std::size_t i = 0; i < count; ++i) pushInstance(items[i].pos, 1.0f, color);
}

void EntityRenderer::drawGems(const glm::vec2 *positions, std::size_t count, const glm::vec3 &color) {
    for (std::size_t i = 0; i < count; ++i)
        pushInstance(glm::vec3(positions[i].x, positions[i].y, 0.5f), 1.0f, color);
}

void EntityRenderer::flushTris() {
    if (m_tris.empty()) return;
    m_triVbo.setData(m_tris.data(), static_cast<uint32_t>(m_tris.size() * sizeof(TriVertex)));
    m_triProg.use();
    RenderCommand::drawArrays(m_triVao, static_cast<uint32_t>(m_tris.size()));
}

void EntityRenderer::flushInstances() {
    if (m_instances.empty()) return;
    m_instanceVbo.setData(m_instances.data(),
                          static_cast<uint32_t>(m_instances.size() * sizeof(InstanceData)));
    m_instProg.use();
    // Small base cube is 36 verts (non-indexed triangle soup).
    RenderCommand::drawInstanced(m_cubeVao, 36, static_cast<uint32_t>(m_instances.size()));
}

void EntityRenderer::flushEnemies() {
    if (m_enemyInstances.empty()) return;
    m_enemyInstanceVbo.setData(m_enemyInstances.data(),
                               static_cast<uint32_t>(m_enemyInstances.size() * sizeof(InstanceData)));
    m_instProg.use();
    // Player-sized enemy base cube is 36 verts.
    RenderCommand::drawInstanced(m_enemyVao, 36, static_cast<uint32_t>(m_enemyInstances.size()));
}

void EntityRenderer::end() {
    flushTris();
    m_tris.clear();
    flushInstances();
    m_instances.clear();
    flushEnemies();
    m_enemyInstances.clear();
    VertexArray::unbind();
}
