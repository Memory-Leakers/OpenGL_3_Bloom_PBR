#ifdef BLOOM

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
uniform sampler2D colorMap;
uniform int maxLod;
layout(location = 0) out vec4 oColor;

void main()
{
    oColor = vec4(0.0);

    for(int lod = 0; lod < maxLod; ++lod)
    {
        oColor += textureLod(colorMap, vTexCoord, float(lod));
    }

	oColor.a = 1.0;
}

#endif
#endif