#version 330

// Implement ambient, diffuse, specular (ADS) shading & lighting

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;

//flat out vec3 LightIntensity;

out vec3 Position;
out vec3 Normal;

/*
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
*/

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;  // inverse transpose of upper-left 3x3 of modelview matrix


// uniforms used for diffuse-only lighting
uniform vec4 LightPosition; // in eye coordinates
uniform vec3 Kd; 			// diffuse reflectivity
uniform vec3 Ld;			// light source intensity


void main()
{
	// convert normal and position to eye coordinates
	Normal = normalize(normalMatrix * vertexNormal);
	Position = vec3((viewMatrix * modelMatrix) * vec4(vertexPosition, 1.0));
	/*
	vec3 tnorm = normalize(normalMatrix * vertexNormal);
	vec4 eyeCoords = (viewMatrix * modelMatrix) * vec4(vertexPosition, 1.0);
	vec3 s = normalize(vec3(Light.Position - eyeCoords));

	vec3 v = normalize(-eyeCoords.xyz);
	vec3 r = reflect(-s, tnorm);

	vec3 ambient = Light.La * Material.Ka;
	float sDotN = max(dot(s, tnorm), 0.0);
	vec3 diffuse = Light.Ld * Material.Kd * sDotN;
	vec3 spec = vec3(0.0);

	if (sDotN > 0.0)
	{
		spec = Light.Ls * Material.Ks * pow(max(dot(r, v), 0.0), Material.Shininess);
	}

	LightIntensity = ambient + diffuse + spec;
	*/
	// diffuse-only shading equation
	//LightIntensity = Light.Ld * Material.Kd * max(dot(s, tnorm), 0.0);

	// convert position to clip coordinates & pass on to next stage
	gl_Position = (projectionMatrix * viewMatrix) * (modelMatrix * vec4(vertexPosition, 1.0));

}