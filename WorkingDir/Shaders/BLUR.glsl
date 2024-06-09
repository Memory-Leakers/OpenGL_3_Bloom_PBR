#ifdef BLUR

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	//vTexCoord = aTexCoord;

    // 顶点坐标
    vec2 vertices[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2(1.0, -1.0),
        vec2(-1.0, 1.0),
        vec2(1.0, 1.0)
    );

    // 通过顶点索引获取顶点坐标
    vec2 position = vertices[gl_VertexID];

    vTexCoord = (position + 1.0) * 2;

	gl_Position = vec4(vTexCoord, 0.0, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform sampler2D colorMap;
uniform vec2 direction;
uniform int inputLod;

in vec2 vTexCoord;
layout(location = 0) out vec4 oColor;

void main()
{
    vec2 texSize = textureSize(colorMap, inputLod);
    vec2 texelSize = 1.0/texSize;
    vec2 margin1 = texelSize * 0.5;
    vec2 margin2 = vec2(1.0) - margin1;

	oColor = vec4(0.0);

    vec2 directionFragCoord = gl_FragCoord.xy * direction;
    int coord = int(directionFragCoord.x + directionFragCoord.y);
    vec2 directionTexSize = texSize * direction;
    int size = int(directionTexSize.x + directionTexSize.y);
    int kernelRadius = 24;
    int kernelBegin = -min(kernelRadius, coord);
    int kernelEnd = min(kernelRadius, size - coord);
    float weight = 0.0;
    for(int i = kernelBegin; i <= kernelEnd; ++i)
    {
        float currentWeight = smoothstep(float(kernelRadius), 0.0, float(abs(i)));
        vec2 finalTexCoords = vTexCoord + i * direction * texelSize;
        finalTexCoords = clamp(finalTexCoords, margin1, margin2);
        oColor += textureLod(colorMap, finalTexCoords, inputLod) * currentWeight;
        weight += currentWeight;
    }

    oColor /= weight;
}

#endif
#endif