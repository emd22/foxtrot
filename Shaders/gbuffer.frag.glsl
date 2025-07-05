#version 450

// Outputs
layout(location = 0) out vec3 OutAlbedo;
// layout(location = 1) out vec3 OutNormal;

// Inputs
layout(location = 0) in vec2 InUV;
// layout(location = 1) in vec3 InNormal;

layout(set = 1, binding = 0) uniform sampler2D TexAlbedo;
// layout(set = 1, binding = 1) uniform sampler2D TexNormal;

// layout(location = 0) out vec4 OutColor;
// layout(location = 1) out vec4 OutNormal;
// layout(location = 2) out vec4 OutPosition;

// layout(location = 0) in vec2 Texcoord;

// layout(set = 1, binding = 1) uniform sampler2D AlbedoSampler;
// layout(set = 1, binding = 2) uniform sampler2D PositionSampler;
// layout(set = 1, binding = 3) uniform sampler2D NormalSampler;

// layout(set = 0, binding = 0) uniform sampler2D Texture;

void main() {
    OutAlbedo = texture(TexAlbedo, InUV).rgb;

    // OutColor = texture(PositionSampler, Texcoord);

    // OutColor = vec4(Texcoord, 0.0, 1.0);
    // OutColor = vec4(Normal.xyz, 1.0);
}
