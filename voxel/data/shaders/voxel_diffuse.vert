#version 330

// Implement basic diffuse lighting using the formula:
// L = Ld x Kd x s . n

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;

flat out vec3 LightIntensity;

uniform vec4 LightPosition; // in eye coordinates
uniform vec3 Kd; 			// diffuse reflectivity
uniform vec3 Ld;			// light source intensity

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;  // inverse transpose of upper-left 3x3 of modelview matrix

void main()
{

	// convert normal and position to eye coordinates
	vec3 tnorm = normalize(normalMatrix * vertexNormal);
	vec4 eyeCoords = (viewMatrix * modelMatrix) * vec4(vertexPosition, 1.0);
	vec3 s = normalize(vec3(LightPosition - eyeCoords));

	// diffuse shading equation
	LightIntensity = Ld * Kd * max(dot(s, tnorm), 0.0);

	// convert position to clip coordinates & pass on to next stage
	gl_Position = (projectionMatrix * viewMatrix) * (modelMatrix * vec4(vertexPosition, 1.0));

}