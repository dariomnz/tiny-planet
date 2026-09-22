#include "graphics/Hud.h"

#include "shaders/Shaders.h"

namespace font5x7 {

void build(const std::string &text, float right, float top, float px,
           std::vector<float> &out) {
  out.clear();
  const float charW = 6.0f * px; // 5 pixels + 1 of spacing
  const float totalW = static_cast<float>(text.size()) * charW;
  const float x0 = right - totalW;
  auto quad = [&](float x, float y, float w, float h) {
    out.insert(out.end(), {x, y, x + w, y, x, y + h,
                           x + w, y, x + w, y + h, x, y + h});
  };
  for (size_t i = 0; i < text.size(); ++i) {
    static const char *G0[] = {"01110", "10001", "10011", "10101",
                               "11001", "10001", "01110"};
    static const char *G1[] = {"00100", "01100", "00100", "00100",
                               "00100", "00100", "01110"};
    static const char *G2[] = {"01110", "10001", "00001", "00110",
                               "01000", "10000", "11111"};
    static const char *G3[] = {"11111", "00010", "00100", "00010",
                               "00001", "10001", "01110"};
    static const char *G4[] = {"00010", "00110", "01010", "10010",
                               "11111", "00010", "00010"};
    static const char *G5[] = {"11111", "10000", "11110", "00001",
                               "00001", "10001", "01110"};
    static const char *G6[] = {"00110", "01000", "10000", "11110",
                               "10001", "10001", "01110"};
    static const char *G7[] = {"11111", "00001", "00010", "00100",
                               "01000", "01000", "01000"};
    static const char *G8[] = {"01110", "10001", "10001", "01110",
                               "10001", "10001", "01110"};
    static const char *G9[] = {"01110", "10001", "10001", "01111",
                               "00001", "00010", "01100"};
    static const char *GF[] = {"11111", "10000", "10000", "11110",
                               "10000", "10000", "10000"};
    static const char *GP[] = {"11110", "10001", "10001", "11110",
                               "10000", "10000", "10000"};
    static const char *GS[] = {"01111", "10000", "10000", "01110",
                               "00001", "00001", "11110"};
    static const char *GM[] = {"10001", "11011", "10101", "10101",
                               "10001", "10001", "10001"};
    static const char *GDOT[] = {"00000", "00000", "00000", "00000",
                                 "00000", "01100", "01100"};
    static const char *GSP[] = {"00000", "00000", "00000", "00000",
                                "00000", "00000", "00000"};
    const char **g = nullptr;
    switch (text[i]) {
    case '0': g = (const char **)G0; break;
    case '1': g = (const char **)G1; break;
    case '2': g = (const char **)G2; break;
    case '3': g = (const char **)G3; break;
    case '4': g = (const char **)G4; break;
    case '5': g = (const char **)G5; break;
    case '6': g = (const char **)G6; break;
    case '7': g = (const char **)G7; break;
    case '8': g = (const char **)G8; break;
    case '9': g = (const char **)G9; break;
    case 'F': g = (const char **)GF; break;
    case 'P': g = (const char **)GP; break;
    case 'S': g = (const char **)GS; break;
    case 'M': g = (const char **)GM; break;
    case '.': g = (const char **)GDOT; break;
    default: g = (const char **)GSP; break;
    }
    const float cx = x0 + static_cast<float>(i) * charW;
    for (int r = 0; r < 7; ++r)
      for (int c = 0; c < 5; ++c)
        if (g[r][c] == '1')
          quad(cx + static_cast<float>(c) * px, top + static_cast<float>(r) * px,
               px, px);
  }
}

} // namespace font5x7

HudRenderer::HudRenderer() : m_prog(shaders::kTextVert, shaders::kTextFrag) {
  m_vao.bind();
  m_vbo.uploadBytes(nullptr, 0, BufferUsage::DynamicDraw);
  m_vao.attrib(0, 2, 2 * sizeof(float), 0);
  VertexArray::unbind();
}

void HudRenderer::setText(const std::string &text) {
  if (text != m_text) {
    m_text = text;
    m_dirty = true;
  }
}

void HudRenderer::setSubText(const std::string &text) {
  if (text != m_subText) {
    m_subText = text;
    m_dirty = true;
  }
}

void HudRenderer::draw(int fbW, int fbH, const glm::vec3 &color) {
  if (m_dirty || fbW != m_fbW || fbH != m_fbH) {
    constexpr float kPx = 3.0f;
    constexpr float kTop = 12.0f;
    constexpr float kRightMargin = 12.0f;
    constexpr float kLineGap = 6.0f; // between line 1 (FPS) and line 2 (MS)
    font5x7::build(m_text, static_cast<float>(fbW) - kRightMargin, kTop, kPx,
                   m_verts);
    if (!m_subText.empty()) {
      std::vector<float> sub;
      font5x7::build(m_subText, static_cast<float>(fbW) - kRightMargin,
                     kTop + 7.0f * kPx + kLineGap, kPx, sub);
      m_verts.insert(m_verts.end(), sub.begin(), sub.end());
    }
    m_vao.bind();
    m_vbo.upload(m_verts, BufferUsage::DynamicDraw);
    VertexArray::unbind();
    m_dirty = false;
    m_fbW = fbW;
    m_fbH = fbH;
  }
  if (m_verts.empty())
    return;
  glDisable(GL_DEPTH_TEST);
  m_prog.use();
  m_prog.set("screenSize",
            glm::vec2(static_cast<float>(fbW), static_cast<float>(fbH)));
  m_prog.set("color", color);
  m_vao.bind();
  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_verts.size() / 2));
  VertexArray::unbind();
  glEnable(GL_DEPTH_TEST);
}
