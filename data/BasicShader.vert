// VERTEX
#version 450 core

layout (location = 0) in vec2 aPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
	gl_Position = uProjection * uView * uModel * vec4(aPosition.xy, 0.0, 1.0);
}