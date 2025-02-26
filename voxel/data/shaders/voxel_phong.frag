#version 330

//flat in vec3 LightIntensity;
in vec3 Position;
in vec3 Normal;

struct LightInfo 
{
	vec4 Position;	// light position in eye coordinates
	vec3 La;		// ambient light intensity
	vec3 Ld;		// diffuse light intensity
	vec3 Ls;		// specular light intensity
};

struct MaterialInfo
{
	vec3 Ka;			// ambient reflectivity
	vec3 Kd;			// diffuse reflectivity
	vec3 Ks;			// specular reflectivity
	float Shininess;	// specular shininess factor
};


uniform LightInfo Light;
uniform MaterialInfo Material;

layout (location = 0) out vec4 FragColor;

vec3 ads()
{
	vec3 n = normalize(Normal);
	vec3 s = normalize(vec3(Light.Position) - Position);
	vec3 v = normalize(vec3(-Position));
	vec3 h = normalize(v + s);

	return Light.La * 
		(Material.Ka + 
		Material.Kd * max(dot(s, Normal), 0.0) + 
		Material.Ks * pow(max(dot(h, n), 0.0), 
		Material.Shininess));
}

void main() 
{
	//FragColor = vec4(LightIntensity, 1.0);
	FragColor = vec4(ads(), 1.0);
}
