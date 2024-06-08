#ifdef PASS_BLIT_BRIGHT

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
uniform sampler2D uTexture;
layout(location = 0) out vec4 oColor;

void main()
{
    vec3 luminances = vec3(0.2126, 0.7152, 0.0722);
    vec4 texel = texture2D(uTexture, vTexCoord);
    float luminance = dot(luminances, texel.rgb);
    luminance = max(0.0, luminance - threshold);
    texel.rgb *= sign(luminance);
    texel.a = 1.0;

	oColor = texel;
}

#endif
#endif