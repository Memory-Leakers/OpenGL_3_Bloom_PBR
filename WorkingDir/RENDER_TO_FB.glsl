///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef RENDER_TO_FB

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

struct Light
{
	uint type;
	vec3 color;
	vec3 direction;
	vec3 position;
	float intensity;
};

layout(binding=0, std140) uniform GlobalParams
{
	vec3 uCamPosition;
	uint uLightCount;
	Light uLight[16];
};

layout(binding=1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

uniform bool useNormalTexture;
uniform sampler2D uNormal;

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vViewDir;

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vViewDir = uCamPosition - vPosition;

	if (useNormalTexture)
	{
		vNormal = vec3(uWorldMatrix * vec4(texture(uNormal, vTexCoord).rgb, 0.0f));
	}
	else
	{
		vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	}
	//vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

struct Light
{
	uint type;
	vec3 color;
	vec3 direction;
	vec3 position;
	float intensity;
};

layout(binding=0, std140) uniform GlobalParams
{
	vec3 uCamPosition;
	uint uLightCount;
	Light uLight[16];
};

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;

uniform sampler2D uTexture;
uniform sampler2D uMetallic;
uniform sampler2D uRoughness;
uniform sampler2D uAO;
uniform sampler2D uEmissive;


layout(location = 0) out vec4 oAlbedo;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oPosition;
layout(location = 3) out vec4 oViewDir;

layout(location = 4) out vec4 oMetallic;
layout(location = 5) out vec4 oRoughness;
layout(location = 6) out vec4 oAo;
layout(location = 7) out vec4 oEmissive;

void main()
{
	oAlbedo = vec4(texture(uTexture, vTexCoord).rgb, 1.0f); //texture(uTexture, vTexCoord);
	oPosition = vec4(vPosition, 1.0);
	oViewDir = vec4(vViewDir, 1.0);
	oNormals = vec4(vNormal, 1.0);


	oMetallic = vec4(vec3(texture(uMetallic, vTexCoord).r), 1.0f);
	oRoughness = vec4(vec3(texture(uRoughness, vTexCoord).r), 1.0f);
	oAo = vec4(vec3(texture(uAO, vTexCoord).r), 1.0f);
	oEmissive = vec4(texture(uEmissive, vTexCoord).rgb, 1.0f);
}

#endif
#endif