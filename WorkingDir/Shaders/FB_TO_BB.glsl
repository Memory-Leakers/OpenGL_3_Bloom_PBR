///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef FB_TO_BB

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

struct Light
{
	uint type;
	vec3 color;
	vec3 direction;
	vec3 position;
	float intensity;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	uint uLightCount;
	Light uLight[16];
};

in vec2 vTexCoord;

uniform sampler2D uAlbedo;
uniform sampler2D uNormals;
uniform sampler2D uPosition;
uniform sampler2D uViewDir;
uniform sampler2D uDepth;

layout(location = 0) out vec4 oColor;

const float PI = 3.14159265359;

uniform bool useEmissive;

// Samplers
uniform sampler2D uMetallic; // Metallic sampler
uniform sampler2D uRoughness; // Roughness sampler
uniform sampler2D uAO; // Ambient Occlusion sampler
uniform sampler2D uEmissive; // Emissive sampler

// Material parameters
vec3 albedo;
float metallic;
float roughness;
float ao;
vec3 normal;
vec3 emissive;

vec3 vPosition;
vec3 vViewDir;
float vDepth;

uniform bool showAlbedo;
uniform bool showNormals;
uniform bool showPosition;
uniform bool showViewDir;
uniform bool showDepth;
uniform bool showMetallic;
uniform bool showRoughness;
uniform bool showAo;
uniform bool showEmissive;

// Lightning Calculation methods
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;
	
	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void SamplerAllTextures()
{
	albedo = texture(uAlbedo, vTexCoord).rgb;
	metallic = texture(uMetallic, vTexCoord).r;
    roughness = texture(uRoughness, vTexCoord).r;
    ao = texture(uAO, vTexCoord).r;
	normal = texture(uNormals, vTexCoord).rgb; //texture normal
	emissive = texture(uEmissive, vTexCoord).rgb;

	vPosition = texture(uPosition, vTexCoord).rgb;
	vViewDir = texture(uViewDir, vTexCoord).rgb;
	vDepth = texture(uDepth, vTexCoord).r;
}

bool SamplerFilter()
{
	if (showAlbedo == true || showDepth == true || showNormals == true || 
	showPosition == true || showViewDir == true || showMetallic == true ||
	showRoughness == true || showAo == true || showEmissive == true)
	{
		if (showAlbedo)
		{
			oColor = vec4(albedo, 1.0);
		}
		else if (showNormals)
		{
			oColor = vec4(normal, 1.0);
		}
		else if (showPosition)
		{
			oColor = vec4(vPosition, 1.0);
		}
		else if (showViewDir)
		{
			oColor = vec4(vViewDir, 1.0);
		}
		else if (showDepth)
		{
			oColor = vec4(vec3(vDepth), 1.0);
		}
		else if (showMetallic)
		{
			oColor = vec4(vec3(metallic), 1.0f);
		}
		else if (showRoughness)
		{
			oColor = vec4(vec3(roughness), 1.0f);
		}
		else if (showAo)
		{
			oColor = vec4(vec3(ao), 1.0f);
		}
		else if (showEmissive)
		{
			oColor = vec4(emissive, 1.0f);
		}

		return true;
	}

	return false;
}

void main()
{
	SamplerAllTextures();

	if (SamplerFilter() == false)
	{
		vec3 N = normalize(normal); //vNormal
		vec3 V = normalize(vViewDir - vPosition); // Podria estar malament

		vec3 F0 = vec3(0.04);
		F0 = mix(F0, albedo, metallic);

		//Reflectance equation
		vec3 Lo = vec3(0.0);
		for (int i = 0; i < uLightCount; ++i)
		{
			vec3 L = vec3(0.0); 
			vec3 radiance = vec3(0.0); 

			if (uLight[i].type == 0)
			{
				// Directional light
				L = normalize(-uLight[i].direction);
				radiance = uLight[i].color * uLight[i].intensity;
			} 
			else 
			{
				// Point light
				L = normalize(uLight[i].position - vPosition);
				float distance = length(uLight[i].position - vPosition);
				float attenuation = 1.0 / (distance * distance);
				radiance = uLight[i].color * (uLight[i].intensity * attenuation);
			}
			
			vec3 H = normalize(V + L);

			float NDF = DistributionGGX(N, H, roughness);
			float G = GeometrySmith(N, V, L, roughness);
			vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

			vec3 kS = F;
			vec3 kD = vec3(1.0) - kS;
			kD *= 1.0 - metallic;

			vec3 numerator = NDF * G * F;
			float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
			vec3 specular = numerator / denominator;

			float NdotL = max(dot(N, L), 0.0);
			Lo += (kD * albedo / PI + specular) * radiance * NdotL;
			
		}

		vec3 ambient = vec3(0.03) * albedo * ao;
		vec3 color = ambient + Lo;
		
		if (useEmissive)
		{
			color += emissive;
		}

		color = color / (color + vec3(1.0));
		color = pow(color, vec3(1.0/2.2));

		oColor =  vec4(color, 1.0);;
	}
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
