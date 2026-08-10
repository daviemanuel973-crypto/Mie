#version 430 core
#ifndef MIE_COMPAT_TEXTURES
#extension GL_ARB_bindless_texture: require
#endif

in flat uvec2 v_textureSampler;
in flat int v_compatTextureIndex;
in vec2 v_uv;
#ifdef MIE_COMPAT_TEXTURES
uniform sampler2D u_compatTextureArray;
uniform int u_compatTextureGrid;
#endif

void main()
{

#ifdef MIE_COMPAT_TEXTURES
	float grid = float(max(u_compatTextureGrid, 1));
    int safeIndex = max(v_compatTextureIndex, 0);
    vec2 tile = vec2(float(safeIndex % u_compatTextureGrid), float(safeIndex / u_compatTextureGrid));
    vec2 localUV = clamp(fract(v_uv), vec2(0.001), vec2(0.999));
    float a = texture(u_compatTextureArray, (tile + localUV) / grid).a;
#else
	float a = texture(sampler2D(v_textureSampler), v_uv).a;
#endif
	if(a <= 0){discard;}

}