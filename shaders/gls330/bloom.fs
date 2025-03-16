#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragPosition;
in vec3 vertexPosition;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 pos;

// Output fragment color
out vec4 finalColor;

void main()
{
    float dist = distance(vertexPosition, fragPosition);
    float mag = 0.5 + dist*dist;
    finalColor = vec4(fragColor.rgb/dist, fragColor.a);
}
