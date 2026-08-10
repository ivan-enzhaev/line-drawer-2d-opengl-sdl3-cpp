// The #version and precision qualifiers are injected by createShaderProgram

out vec4 fragColor;

uniform vec3 uColor;

void main()
{
    fragColor = vec4(uColor, 1.0);
}
