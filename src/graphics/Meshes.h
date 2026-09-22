#pragma once

#include <vector>

#include "gl/Types.h"

// Pure geometry factories (no OpenGL). VBO upload is done by the Renderer.
namespace meshes {

std::vector<Vertex2D> makePlanetGrid(int cells, float cell);

// Generic box [x0,x1]x[y0,y1]x[z0,z1] with per-face normals (36 vertices).
std::vector<VertexPN> makeBox(float x0, float x1, float y0, float y1, float z0,
                              float z1);

inline std::vector<VertexPN> makePlayerCube() {
  return makeBox(-0.5f, 0.5f, -0.5f, 0.5f, 0.0f, 2.0f);
}

inline std::vector<VertexPN> makeNose() {
  return makeBox(0.5f, 0.75f, -0.25f, 0.25f, 1.1f, 1.6f);
}

inline std::vector<VertexPN> makeProjectileCube(float size) {
  const float h = size * 0.5f;
  return makeBox(-h, h, -h, h, -h, h);
}

} // namespace meshes
