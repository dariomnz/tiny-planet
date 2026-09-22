#include "graphics/PlanetRenderer.h"

#include "Config.h"
#include "gl/Types.h"
#include "graphics/Meshes.h"
#include "shaders/Shaders.h"

PlanetRenderer::PlanetRenderer() : m_prog(shaders::kPlanetVert, shaders::kPlanetFrag) {
    const auto grid = meshes::makePlanetGrid(config::kCells, config::kCell);
    m_count = static_cast<GLsizei>(grid.size());
    m_vao.bind();
    m_vbo.upload(grid, BufferUsage::StaticDraw);
    m_vao.attrib(0, 2, sizeof(Vertex2D), offsetof(Vertex2D, pos));
    m_vao.attrib(1, 3, sizeof(Vertex2D), offsetof(Vertex2D, bary));
    VertexArray::unbind();
}

void PlanetRenderer::draw(const glm::mat4 &mvp, const glm::vec2 &playerPos, const glm::vec2 &gridOffset, float curveK,
                          float fill, const glm::vec3 &edgeColor, float fogDensity) const {
    m_prog.use();
    m_prog.set("mvp", mvp);
    m_prog.set("playerPos", playerPos);
    m_prog.set("gridOffset", gridOffset);
    m_prog.set("curveK", curveK);
    m_prog.set("fill", fill);
    m_prog.set("edgeColor", edgeColor);
    m_prog.set("fogDensity", fogDensity);
    m_vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, m_count);
}
