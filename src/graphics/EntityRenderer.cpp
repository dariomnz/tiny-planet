#include "graphics/EntityRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Config.h"
#include "gl/Types.h"
#include "graphics/Meshes.h"
#include "shaders/Shaders.h"
#include "world/Projectiles.h"

namespace {
void uploadMesh(const std::vector<VertexPN> &mesh, VertexArray &vao, Buffer &vbo, GLsizei &count) {
    count = static_cast<GLsizei>(mesh.size());
    vao.bind();
    vbo.upload(mesh, BufferUsage::StaticDraw);
    vao.attrib(0, 3, sizeof(VertexPN), offsetof(VertexPN, pos));
    vao.attrib(1, 3, sizeof(VertexPN), offsetof(VertexPN, normal));
    VertexArray::unbind();
}
}  // namespace

EntityRenderer::EntityRenderer() : m_prog(shaders::kEntityVert, shaders::kEntityFrag) {
    uploadMesh(meshes::makePlayerCube(), m_playerVao, m_playerVbo, m_playerCount);
    uploadMesh(meshes::makeNose(), m_noseVao, m_noseVbo, m_noseCount);
    uploadMesh(meshes::makeProjectileCube(config::kProjSize), m_projVao, m_projVbo, m_projCount);
}

void EntityRenderer::begin(const glm::mat4 &projView, const glm::vec2 &playerPos, float curveK) const {
    m_prog.use();
    m_prog.set("projView", projView);
    m_prog.set("playerPos", playerPos);
    m_prog.set("curveK", curveK);
}

void EntityRenderer::drawPlayer(const glm::mat4 &model, const glm::vec3 &color) const {
    m_prog.set("model", model);
    m_prog.set("color", color);
    m_playerVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, m_playerCount);
}

void EntityRenderer::drawNose(const glm::mat4 &model, const glm::vec3 &color) const {
    m_prog.set("model", model);
    m_prog.set("color", color);
    m_noseVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, m_noseCount);
}

void EntityRenderer::drawProjectiles(const Projectile *items, std::size_t count, const glm::vec3 &color) const {
    m_prog.set("color", color);
    m_projVao.bind();
    for (std::size_t i = 0; i < count; ++i) {
        const glm::mat4 m = glm::translate(glm::mat4(1.0f), items[i].pos);
        m_prog.set("model", m);
        glDrawArrays(GL_TRIANGLES, 0, m_projCount);
    }
    VertexArray::unbind();
}

void EntityRenderer::drawEnemies(const glm::vec2 *positions, const float *scales, std::size_t count,
                                 const glm::vec3 &color) const {
    // Reuses the player cube mesh: only color/scale differ (M1: red chasers).
    // Same curved shader, no per-frame allocs.
    m_prog.set("color", color);
    m_playerVao.bind();
    for (std::size_t i = 0; i < count; ++i) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(positions[i].x, positions[i].y, 0.0f));
        const float s = scales ? scales[i] : 1.0f;
        m = glm::scale(m, glm::vec3(s, s, s));
        m_prog.set("model", m);
        glDrawArrays(GL_TRIANGLES, 0, m_playerCount);
    }
    VertexArray::unbind();
}
