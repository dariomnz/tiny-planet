#pragma once

#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "gl/Buffer.h"
#include "gl/Program.h"
#include "gl/VertexArray.h"

// 5x7 bitmap font (only "0123456789FPSM. "). Pure geometry, no OpenGL.
namespace font5x7 {
void build(const std::string &text, float right, float top, float px,
           std::vector<float> &out);
} // namespace font5x7

// 2D HUD in pixels with top-left origin. Rebuild only when dirty.
class HudRenderer {
public:
  HudRenderer();

  void setText(const std::string &text);
  // Second line below the main one (e.g. "16.6 MS"). Empty = hidden.
  void setSubText(const std::string &text);
  void draw(int fbW, int fbH, const glm::vec3 &color);

private:
  Program m_prog;
  VertexArray m_vao;
  Buffer m_vbo;
  std::vector<float> m_verts;
  std::string m_text;
  std::string m_subText;
  int m_fbW = 0;
  int m_fbH = 0;
  bool m_dirty = true;
};
