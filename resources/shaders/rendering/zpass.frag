#version 430 core
#ifndef MIE_COMPAT_TEXTURES
#extension GL_ARB_bindless_texture: require
#endif

in flat uvec2 v_textureSampler;
in flat int v_compatTextureIndex;
in vec2 v_uv;
#ifdef MIE_COMPAT_TEXTURES
uniform sampler2DArray u_compatTextureArray;
#endif

void main()
{

#ifdef MIE_COMPAT_TEXTURES
	float a = texture(u_compatTextureArray, vec3(v_uv, float(v_compatTextureIndex))).a;
#else
	float a = texture(sampler2D(v_textureSampler), v_uv).a;
#endif
	if(a <= 0){discard;}

}