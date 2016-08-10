
#version 330 core

in vec2 vertex;

smooth out vec2 uv;

void main(){
	gl_Position = vec4(vertex, 0.0, 1.0);
	
	uv = vertex;
}
