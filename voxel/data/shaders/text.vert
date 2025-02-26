#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec4 vertexColor;
layout(location = 3) in float vertexShift;
layout(location = 4) in float vertexGamma;

out float fragShift;
out float fragGamma;
smooth out vec4 fragColor;
out vec2 fragUV;

uniform sampler2D texture;
uniform vec3 pixel;
uniform mat4 MVPMatrix;

void main()
{
    //gl_FrontColor = gl_Color;
    //gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;

	gl_Position = MVPMatrix * vec4(vertexPosition, 1.0);

	fragShift = vertexShift;
	fragGamma = vertexGamma;
	fragColor = vertexColor;
	fragUV = vertexUV;
}
