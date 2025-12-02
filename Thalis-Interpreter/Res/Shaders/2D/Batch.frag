#version 450

layout(location = 0) in vec2 TexCoord0;
layout(location = 1) in vec4 Color0;
layout(location = 2) flat in float TextureIndex0;

layout(location = 0) out vec4 OutColor;

uniform sampler2D Textures[32];

void main()
{
    OutColor =  texture(Textures[int(TextureIndex0)], TexCoord0) * Color0;
}