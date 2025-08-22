#version 430 core

layout(location = 0) in vec2 uv;

layout(binding = 0) uniform sampler2D uTexture;
layout(binding = 1) uniform UBO {
    vec4 color;
} ubo;

layout(location = 0) out vec4 fragColor;

void main() {
  fragColor = texture(uTexture, uv) * ubo.color;
}
