#version 450

layout(location = 0) in vec2 Pos;
layout(location = 1) in vec2 TexCoord;
layout(location = 2) in vec4 Color;
layout(location = 3) in float TextureIndex;

layout(location = 0) out vec2 TexCoord0;
layout(location = 1) out vec4 Color0;
layout(location = 2) flat out float TextureIndex0;

uniform mat4 ViewProjection;

void main()
{
    gl_Position = vec4(Pos, 0.0, 1.0) * ViewProjection;

    TexCoord0 = TexCoord;
    Color0 = Color;
    TextureIndex0 = TextureIndex;
}