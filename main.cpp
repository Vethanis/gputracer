
#include "myglheaders.h"
#include "camera.h"
#include "debugmacro.h"
#include "window.h"
#include "input.h"
#include "glprogram.h"
#include "compute_shader.h"
#include "glscreen.h"
#include "UBO.h"
#include "SSBO.h"
#include "timer.h"
#include "texture.h"
#include "time.h"
#include <random>
#include "string.h"

using namespace std;
using namespace glm;

struct Uniforms{
	mat4 IVP;
	vec4 eye;
	vec4 nfwh;
    vec4 seed;
};

struct Material{
    vec4 reflectance, emittance;
};

struct SDF_BUF{
    Material materials[5];
};

float frameBegin(unsigned& i, float& t){
    float dt = (float)glfwGetTime() - t;
    t += dt;
    i++;
    if(t >= 3.0f){
    	float ms = (t / i) * 1000.0f;
        printf("ms: %.6f, FPS: %.3f\n", ms, i / t);
        i = 0;
        t = 0.0f;
        glfwSetTime(0.0);
    }
    return dt;
}
#define MIN(a, b) ((a) < (b) ? (a) : (b))

char* nextline(char* p){
    while(*p != EOF && *p != '\n')
        p++;
    if(*p != EOF)
        p++;
    return p;
}

void read_map(char* p, float* buf){
    int mats = atoi(p);
    p = nextline(p);
    Material mat;
    for(int i = 0; i < mats; i++){
        for(int j = 0; j < 8; j++, buf++){
            *buf = (float)atof(p);
            p = nextline(p);
        }
    }
}

char* copy_file(const char* filename){
    FILE* file = fopen(filename, "rb");
    if(!file) return nullptr;
	
    size_t filesize;
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    rewind(file);
    char* fbuf = (char*)malloc(filesize);
    fread(fbuf, 1, filesize, file);
    fclose(file);
    return fbuf;
}

int main(int argc, char* argv[]){
    srand(time(NULL));
    int WIDTH = 1280, HEIGHT = 720;
	if(argc != 3){
        WIDTH = atoi(argv[1]);
        HEIGHT = atoi(argv[2]);
	}
    
	SDF_BUF sdf_buf;
    char* fbuf = copy_file("map.txt");
    if(!fbuf){
        puts("Cannot open map.txt");
        return 1;
    }
    read_map(fbuf, (float*)&sdf_buf);
    free(fbuf);
    
	Camera camera;
	camera.resize(WIDTH, HEIGHT);
	camera.setEye(vec3(0.0f, 0.0f, 1.9f));
	camera.update();
	
	const unsigned layoutSize = 8;
	const unsigned callsizeX = WIDTH / layoutSize + ((WIDTH % layoutSize) ? 1 : 0);
	const unsigned callsizeY = HEIGHT / layoutSize + ((HEIGHT % layoutSize) ? 1 : 0);
	
	Window window(WIDTH, HEIGHT, 4, 3, "Meshing");
	Input input(window.getWindow());
	
	GLProgram color("vert.glsl", "frag.glsl");
	ComputeShader depth("depth.glsl");
	Texture4f colTex(WIDTH, HEIGHT);
	colTex.setCSBinding(0);
	GLScreen screen;
	Timer timer;
	
	Uniforms uni;
	uni.IVP = camera.getIVP();
	uni.eye = vec4(camera.getEye(), 1.0f);
	uni.nfwh = vec4(camera.getNear(), camera.getFar(), (float)WIDTH, (float)HEIGHT);
	UBO unibuf(&uni, sizeof(uni), 2);
    
	SSBO sdfbuf(&sdf_buf, sizeof(sdf_buf), 3);
	
	input.poll();
    unsigned i = 0;
    float frame = 1.0f;
    float irm = 1.0f / RAND_MAX;
    float t = (float)glfwGetTime();
    while(window.open()){
        input.poll(frameBegin(i, t), camera);
        if(input.leftMouseDown())
            frame = 1;
        
        uni.IVP = camera.getIVP();
        uni.eye = vec4(camera.getEye(), 1.0f);
        uni.seed = vec4(rand() * irm, rand() * irm, rand() * irm, frame);
        unibuf.upload(&uni, sizeof(uni));
        
		depth.bind();
		depth.call(callsizeX, callsizeY, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		
		color.bind();
		colTex.bind(0, "color", color);
		screen.draw();
		
        window.swap();
        frame = MIN(frame + 1.0f, 100000.0f);
    }

    return 0;
}

