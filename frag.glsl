#version 430 core

smooth in vec2 uv;

uniform sampler2D color;

out vec4 out_color;

void main(){
	vec2 suv = uv * 0.5f + 0.5f;
	vec3 mat = texture(color, suv).rgb;
    mat = pow(mat, vec3(1.0 / 2.2));
	out_color = vec4(mat, 1.0f);
}
