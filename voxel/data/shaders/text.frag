#version 330

uniform sampler2D texture;
uniform vec3 pixel;

smooth in vec4 fragColor;
in float fragGamma;
in float fragShift;
in vec2 fragUV;

out vec4 theColor;

void main() {

    // LCD Off
    if( pixel.z == 1.0)
    {
        //vec2 uv = gl_TexCoord[0].xy;
        vec2 uv = fragUV;
        float a = texture2D(texture, uv).a;
        theColor = fragColor * pow(a, 1.0 / fragGamma);
        return;
    }

    // LCD On
    //vec2 uv      = gl_TexCoord[0].xy;
    vec2 uv = fragUV;
    vec4 current = texture2D(texture, uv);
    vec4 previous= texture2D(texture, uv+vec2(-1,0)*pixel.xy);
    vec4 next    = texture2D(texture, uv+vec2(+1,0)*pixel.xy);

    float r = current.r;
    float g = current.g;
    float b = current.b;

    if (fragShift <= 0.333)
    {
        float z = fragShift/0.333;
        r = mix(current.r, previous.b, z);
        g = mix(current.g, current.r,  z);
        b = mix(current.b, current.g,  z);
    } 
    else if (fragShift <= 0.666)
    {
        float z = (fragShift-0.33)/0.333;
        r = mix(previous.b, previous.g, z);
        g = mix(current.r,  previous.b, z);
        b = mix(current.g,  current.r,  z);
    }
    else if (fragShift < 1.0)
    {
        float z = (fragShift-0.66)/0.334;
        r = mix(previous.g, previous.r, z);
        g = mix(previous.b, previous.g, z);
        b = mix(current.r,  previous.b, z);
    }

    vec3 color = pow( vec3(r,g,b), vec3(1.0/fragGamma));
    theColor.rgb = color * fragColor.rgb;
    theColor.a = (color.r + color.g + color.b) / 3.0 * fragColor.a;
}
