layout (location = 0) in vec4 VertCoords;
layout (location = 1) in vec4 Spheres;

out vec4 spheres;

uniform mat4 projection;

void main()
{
    spheres = Spheres;
    gl_Position = projection * vec4(VertCoords.xy, 0.0, 1.0);
}
