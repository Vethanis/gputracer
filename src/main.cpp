
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
#include <fstream>

#include "image.h"

using namespace std;
using namespace glm;

struct Uniforms{
    glm::mat4 IVP;
    glm::vec4 eye;
    glm::vec4 nfwh;
    glm::vec4 seed;
};

struct SDF{
    vec4 position; // xyz -> position w -> sdf type
    vec4 scale; // xyz -> scale w -> blend type
    vec4 rotation; // quaternion
    vec4 parameters; 
    // x -> object id 
    // y -> material id 
    // z -> blend smoothness
    void set_distance_type(s32 t) { position.w = f32(t); }
    void set_blend_type(s32 t){ scale.w = f32(t); }
    void set_object_id(s32 i){ parameters.x = f32(i); }
    void set_material_id(s32 i){ parameters.y = f32(i); }
    void set_blend_smoothness(f32 s){ parameters.z = s; }
};

struct SDF_Edits{
    SDF sdfs[1024];
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
#define MAX(a, b) ((a) > (b) ? (a) : (b))

bool read_map(float* buf, int len, const char* filename){
    ifstream fs(filename);
    if(!fs.is_open())return false;
    
    for(int i = 0; i < len && fs.good(); i++){
        fs >> buf[i];
    }
    
    fs.close();
    return true;
}

inline float absSum(const glm::vec3& v){
    return fabsf(v.x) + fabsf(v.y) + fabsf(v.z);
}

bool v3_equal(const glm::vec3& a, const glm::vec3& b){
    return absSum(a - b) == 0.0f;
}

int main(int argc, char* argv[]){
    srand((unsigned)time(NULL));
    int WIDTH = 1280, HEIGHT = 720;
    if(argc == 3){
        WIDTH = atoi(argv[1]);
        HEIGHT = atoi(argv[2]);
    }
    
    SDF_Edits sdf_edits;

    Texture4uc textures[] = {
        Texture4uc("reflectance.png"),
        Texture4uc("emittance.png"),
        Texture4uc("normal.png")
    };
    
    Camera camera;
    camera.resize(WIDTH, HEIGHT);
    camera.setEye({-1.0f, 4.0f, 10.0f});
    camera.lookAt({0.0f, 0.0f, 0.0f});
    camera.update();
    
    const unsigned layoutSize = 8;
    const unsigned callsizeX = WIDTH / layoutSize + ((WIDTH % layoutSize) ? 1 : 0);
    const unsigned callsizeY = HEIGHT / layoutSize + ((HEIGHT % layoutSize) ? 1 : 0);
    
    Window window(WIDTH, HEIGHT, 4, 3, "gputracer");
    Input input(window.getWindow());
    
    GLProgram color("vert.glsl", "frag.glsl");
    ComputeShader depth("depth.glsl");
    Texture4f colTex(WIDTH, HEIGHT);
    colTex.setCSBinding(0);
    GLScreen screen;
    Timer timer;
    
    Uniforms uni;
    uni.IVP = camera.getIVP();
    uni.eye = glm::vec4(camera.getEye(), 1.0f);
    uni.nfwh = glm::vec4(camera.getNear(), camera.getFar(), (float)WIDTH, (float)HEIGHT);
    UBO unibuf(&uni, sizeof(uni), 2);
    
    SSBO sdfbuf(&sdf_edits, sizeof(SDF_Edits), 3);
    
    input.poll();
    unsigned i = 0;
    float frame = 1.0f;
    float irm = 1.0f / RAND_MAX;
    float t = (float)glfwGetTime();
    while(window.open()){
        glm::vec3 eye = camera.getEye();
        glm::vec3 at = camera.getAt();
        input.poll(frameBegin(i, t), camera);
        if(!v3_equal(eye, camera.getEye()) || !v3_equal(at, camera.getAt()))
            frame = 2;
        
        if(((int)(frame) & 31) == 31)
            printf("SPP: %f\n", frame);
        
        uni.IVP = camera.getIVP();
        uni.eye = glm::vec4(camera.getEye(), 1.0f);
        uni.seed = glm::vec4(rand() * irm, rand() * irm, rand() * irm, frame);
        unibuf.upload(&uni, sizeof(uni));
        
        depth.bind();
        depth.call(callsizeX, callsizeY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        
        color.bind();
        colTex.bind(0, "color", color);
        screen.draw();
        
        window.swap();
        frame = MIN(frame + 1.0f, 1000000.0f);
    }

    return 0;
}

