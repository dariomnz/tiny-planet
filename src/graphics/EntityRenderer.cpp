#include "graphics/EntityRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Config.h"
#include "gl/Types.h"
#include "graphics/Meshes.h"
#include "shaders/Shaders.h"
#include "world/Projectiles.h"

namespace {
void uploadMesh(const std::vector<VertexPN> &mesh, VertexArray &vao,
                Buffer &vbo, GLsizei &count) {
  count = static_cast<GLsizei>(mesh.size());
  vao.bind();
  vbo.upload(mesh, BufferUsage::StaticDraw);
  vao.attrib(0, 3, sizeof(VertexPN), offsetof(VertexPN, pos));
  vao.attrib(1, 3, sizeof(VertexPN), offsetof(VertexPN, normal));
  VertexArray::unbind();
}
} // namespace

EntityRenderer::EntityRenderer()
    : m_prog(shaders::kEntityVert, shaders::kEntityFrag) {
  uploadMesh(meshes::makePlayerCube(), m_playerVao, m_playerVbo, m_playerCount);
  uploadMesh(meshes::makeNose(), m_noseVao, m_noseVbo, m_noseCount);
  uploadMesh(meshes::makeProjectileCube(config::kProjSize), m_projVao, m_projVbo,
             m_projCount);
}

void EntityRenderer::begin(const glm::mat4 &projView,
                           const glm::vec2 &playerPos, float curveK) const {
  m_prog.use();
  m_prog.set("projView", projView);
  m_prog.set("playerPos", playerPos);
  m_prog.set("curveK", curveK);
}

void EntityRenderer::drawPlayer(const glm::mat4 &model,
                                const glm::vec3 &color) const {
  m_prog.set("model", model);
  m_prog.set("color", color);
  m_playerVao.bind();
  glDrawArrays(GL_TRIANGLES, 0, m_playerCount);
}

void EntityRenderer::drawNose(const glm::mat4 &model,
                              const glm::vec3 &color) const {
  m_prog.set("model", model);
  m_prog.set("color", color);
  m_noseVao.bind();
  glDrawArrays(GL_TRIANGLES, 0, m_noseCount);
}

void EntityRenderer::drawProjectiles(const std::vector<Projectile> &items,
                                     const glm::vec3 &color) const {
  m_prog.set("color", color);
  m_projVao.bind();
  for (const Projectile &p : items) {
    const glm::mat4 m = glm::translate(glm::mat4(1.0f), p.pos);
    m_prog.set("model", m);
    glDrawArrays(GL_TRIANGLES, 0, m_projCount);
  }
  VertexArray::unbind();
}
