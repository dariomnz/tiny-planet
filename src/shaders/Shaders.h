#pragma once

// Canonical editable source: src/shaders/*.glsl.
// On web (Emscripten) there is no runtime filesystem, so the content
// is embedded here as string_view. Keep in sync with the .glsl files.

#include <string_view>

namespace shaders {

inline constexpr std::string_view kPlanetVert = R"GLSL(#version 300 es
precision highp float;
layout (location = 0) in vec2 inPos;
layout (location = 1) in vec3 inBary;
out vec3 vBary;
out float vDist;
uniform mat4 mvp;
uniform vec2 playerPos;   // sinking is measured from the player
uniform vec2 gridOffset;  // grid snap (cell multiple)
uniform float curveK;     // 0 = flat, higher = smaller planet
void main() {
    vec2 world = inPos + gridOffset;
    vec2 rel = world - playerPos;
    float z = -curveK * dot(rel, rel); // farther from player = more sunken
    gl_Position = mvp * vec4(world.x, world.y, z, 1.0);
    vBary = inBary;
    vDist = length(rel);
}
)GLSL";

inline constexpr std::string_view kPlanetFrag = R"GLSL(#version 300 es
precision mediump float;
in vec3 vBary;
in float vDist;
out vec4 outColor;
uniform float fill;       // 0 = edges only, 1 = solid triangle
uniform vec3 edgeColor;
uniform float fogDensity; // distance attenuation
void main() {
    float d = min(vBary.x, min(vBary.y, vBary.z)); // 0 on edge, ~0.33 center
    float w = fwidth(d) + 1e-6;
    float t = mix(w * 1.5, 0.5, fill);
    float a = 1.0 - smoothstep(t - w, t, d);
    float fog = exp(-vDist * fogDensity);
    outColor = vec4(edgeColor * a * fog, 1.0);
}
)GLSL";

inline constexpr std::string_view kEntityVert = R"GLSL(#version 300 es
precision highp float;
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
out vec3 vNormal;
uniform mat4 projView;
uniform mat4 model;
uniform vec2 playerPos;
uniform float curveK;
void main() {
    vec4 world = model * vec4(inPos, 1.0);
    vec2 rel = world.xy - playerPos;
    world.z -= curveK * dot(rel, rel); // same formula as planetVertSrc
    gl_Position = projView * world;
    vNormal = inNormal; // the model only translates/rotates on Z: normals stay unchanged
}
)GLSL";

inline constexpr std::string_view kEntityFrag = R"GLSL(#version 300 es
precision mediump float;
in vec3 vNormal;
out vec4 outColor;
uniform vec3 color;
void main() {
    vec3 lightDir = normalize(vec3(0.4, 0.5, 0.75));
    float shade = 0.45 + 0.55 * max(dot(normalize(vNormal), lightDir), 0.0);
    outColor = vec4(color * shade, 1.0);
}
)GLSL";

// Batched paths share one Frame UBO (binding 0), set once in begin().
// Portable GLSL ES 3.0: no `binding=` qualifier here; C++ binds the
// "Frame" block index to 0 via glUniformBlockBinding.
inline constexpr std::string_view kFrameUbo = R"GLSL(
layout(std140) uniform Frame {
    mat4 projView;
    vec2 playerPos;
    float curveK;
    float _pad;
};
)GLSL";

// Instanced cubes: base mesh (divisor 0) + offset/scale/color (divisor 1).
// One DrawInstanced per batch instead of one draw per entity.
inline constexpr std::string_view kEntityInstVert = R"GLSL(#version 300 es
precision highp float;
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inOffset;
layout (location = 3) in float inScale;
layout (location = 4) in vec3 inColor;
layout(std140) uniform Frame {
    mat4 projView;
    vec2 playerPos;
    float curveK;
    float _pad;
};
out vec3 vNormal;
out vec3 vColor;
void main() {
    vec4 world = vec4(inPos * inScale + inOffset, 1.0);
    vec2 rel = world.xy - playerPos;
    world.z -= curveK * dot(rel, rel);
    gl_Position = projView * world;
    vNormal = inNormal;
    vColor = inColor;
}
)GLSL";

inline constexpr std::string_view kEntityInstFrag = R"GLSL(#version 300 es
precision mediump float;
in vec3 vNormal;
in vec3 vColor;
out vec4 outColor;
void main() {
    vec3 lightDir = normalize(vec3(0.4, 0.5, 0.75));
    float shade = 0.45 + 0.55 * max(dot(normalize(vNormal), lightDir), 0.0);
    outColor = vec4(vColor * shade, 1.0);
}
)GLSL";

// Merged unique triangles: arbitrary meshes, CPU-transformed, per-vertex color.
inline constexpr std::string_view kEntityTriVert = R"GLSL(#version 300 es
precision highp float;
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout(std140) uniform Frame {
    mat4 projView;
    vec2 playerPos;
    float curveK;
    float _pad;
};
out vec3 vNormal;
out vec3 vColor;
void main() {
    vec4 world = vec4(inPos, 1.0);
    vec2 rel = world.xy - playerPos;
    world.z -= curveK * dot(rel, rel);
    gl_Position = projView * world;
    vNormal = inNormal;
    vColor = inColor;
}
)GLSL";

}  // namespace shaders
