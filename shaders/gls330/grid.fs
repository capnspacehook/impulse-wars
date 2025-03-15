#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

// Input uniform values
uniform vec2 pos[4];
uniform vec4 color[4];

// Output fragment color
out vec4 finalColor;

void main()
{
    finalColor = fragColor;

    // Texel color fetching from texture sampler
    for (int i = 0; i < 4; i++) {
        vec2 p = pos[i];
        vec4 c = color[i];
        vec2 origin = vec2(0.0, 0.0);
        //float dist = distance(p, fragTexCoord);
        float dist = distance(p, fragPosition.xz);
        float mag = 9.0/(dist*dist);
        finalColor.r += mag*c.r/255.0;
        finalColor.g += mag*c.g/255.0;
        finalColor.b += mag*c.b/255.0;
        finalColor.a = 255.0;
    }
}

