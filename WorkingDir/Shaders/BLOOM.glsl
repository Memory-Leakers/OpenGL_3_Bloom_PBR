#ifdef BLOOM

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	  // square
     vec2 vertices[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2(1.0, -1.0),
        vec2(-1.0, 1.0),
        vec2(1.0, 1.0)
    );

    // get corresponding position
    vec2 position = vertices[gl_VertexID];

    vTexCoord = (position + 1.0);

	gl_Position = vec4(vTexCoord, 0.0, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
uniform sampler2D colorMap;
uniform int maxLod;
uniform float lodIntensity[5];

layout(location = 0) out vec4 oColor;

void main()
{
    oColor = vec4(0.0);

    for(int lod = 0; lod < maxLod; ++lod)
    {
          oColor += textureLod(colorMap, vTexCoord, float(lod)) * lodIntensity[lod];
    }

	oColor.a = 1.0;

    //oColor = vec4(1.0, 0.0, 0.0, 1.0);


    // Learn OpenGL method
    //const float gamma = 2.2;
    //vec3 hdrColor = texture(colorMap, vTexCoord).rgb;      
    //vec3 bloomColor = texture(blurMap, vTexCoord).rgb;
    //hdrColor += bloomColor; // additive blending
    // tone mapping
    //vec3 result = vec3(1.0) - exp(-hdrColor * 0.5);
    // also gamma correct while we're at it       
    //result = pow(result, vec3(1.0 / gamma));
    
    //oColor = vec4(1.0, 0.0, 0.0, 1.0);
}

#endif
#endif