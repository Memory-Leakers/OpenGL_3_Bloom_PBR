///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef RENDER_TO_BB

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
	vec3 uCameraPosition;
	uint uLightCount;
	Light uLight[16];
};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vViewDir;
out mat4 vWorldMatrix;

layout(binding=1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vViewDir = uCameraPosition - vPosition;
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vWorldMatrix = uWorldMatrix;

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0f);
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
	vec3 uCameraPosition;
	uint uLightCount;
	Light uLight[16];
};

layout(location = 0) out vec4 oColor;

const float PI = 3.14159265359;

in vec2 vTexCoord; // Texture Coords
in vec3 vPosition; // World Position
in vec3 vNormal; // Normal
in vec3 vViewDir; // View Direction // Camera Position
in mat4 vWorldMatrix;

uniform bool useEmissive;
uniform bool useNormalTexture;

uniform bool usePBR;

// Samplers
uniform sampler2D uTexture; // Albedo sampler
uniform sampler2D uMetallic; // Metallic sampler
uniform sampler2D uRoughness; // Roughness sampler
uniform sampler2D uAO; // Ambient Occlusion sampler
uniform sampler2D uNormal; // Normal sampler
uniform sampler2D uEmissive; // Emissive sampler

// Material parameters
vec3 albedo;
float metallic;
float roughness;
float ao;
vec3 emissive;

vec3 normal;

// SAMPLE TEXTURES
void SamplerAllTextures()
{
	albedo = texture(uTexture, vTexCoord).rgb;
	
	if (usePBR)
	{
		metallic = texture(uMetallic, vTexCoord).r;
		roughness = texture(uRoughness, vTexCoord).r;
		ao = texture(uAO, vTexCoord).r;
		emissive = texture(uEmissive, vTexCoord).rgb;

		// Sample normal texture if there is one
		if(useNormalTexture)
		{
			normal = texture(uNormal, vTexCoord).rgb;
			normal = vec3(vWorldMatrix * vec4(normal, 0.0f));
		}
		else
		{
			normal = vNormal;
		}
	}
}

// PBR METHOD
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

// PBR METHOD
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

// PBR METHOD
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

// PBR METHOD
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	//return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// MAIN PBR
void CalculatePBRLightning()
{
	vec3 N = normalize(normal); //vNormal
	vec3 V = normalize(vViewDir - vPosition); 

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo.rgb, metallic);

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

	oColor = vec4(color, 1.0f);	
}

// OLD LIGHTNING (NON PBR METHOD)
void CalculateBlitVars(in Light light, out vec3 ambient, out vec3 diffuse, out vec3 specular)
{
	vec3 lightDir = normalize(light.direction);

	float ambientStrength = 0.2;
	ambient = ambientStrength * light.color;

	float diff = max(dot(vNormal, lightDir), 0.0f);
	diffuse = diff * light.color;

	float specularStrength = 0.1f;
	vec3 reflectDir = reflect(-lightDir, vNormal);
	vec3 normalViewDir = normalize(vViewDir);
	float spec = pow(max(dot(normalViewDir, reflectDir), 0.0f), 32);
	specular = specularStrength * spec * light.color;
}

// MAIN OLD LIGHTNING (NON PBR)
void CalculateBasicLightning()
{
	vec4 textureColor = texture(uTexture, vTexCoord);
	vec4 finalColor = vec4(0.0f);
	for (int i = 0; i < uLightCount; ++i)
	{
		vec3 lightResult = vec3(0.0f);
		vec3 ambient = vec3(0.0f);
		vec3 diffuse = vec3(0.0f);
		vec3 specular = vec3(0.0f);

		if (uLight[i].type == 0)
		{
			//Directional
			Light light = uLight[i];

			CalculateBlitVars(light, ambient, diffuse, specular);

			lightResult = ambient + diffuse + specular;
			finalColor += vec4(lightResult, 1.0) * textureColor * (light.intensity/2);
		}
		else
		{
			Light light = uLight[i];

			float constant = 1.0f;
			float linear = 0.09f;
			float quadratic = 0.032f;
			float distance = length(light.position - vPosition);
			float attenuation = 1.0f / (constant + linear * distance + quadratic * (distance*distance));

			CalculateBlitVars(light, ambient, diffuse, specular);
			lightResult = (ambient * attenuation) + (diffuse * attenuation) + (specular * attenuation);
			finalColor += vec4(lightResult, 1.0) * textureColor * (light.intensity/2);
		}
	}

	oColor = finalColor;
}

void main()
{
	SamplerAllTextures();
	
	if (usePBR)
	{
		// PBR Lightning
		CalculatePBRLightning();
	}
	else
	{
		// Basic Lightning
		CalculateBasicLightning();
	}
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
