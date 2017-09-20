
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
#include "image.h"
#include <glm/gtx/euler_angles.hpp> 

#define SDF_SPHERE 0
#define SDF_BOX 1
#define SDF_PLANE 2
#define SDF_CONE 3
#define SDF_PYRAMID 4
#define SDF_TORUS 5
#define SDF_CYLINDER 6
#define SDF_CAPSULE 7
#define SDF_DISK 8
#define SDF_TYPE_COUNT 9

#define SDF_BLEND_UNION 0
#define SDF_BLEND_DIFF 1
#define SDF_BLEND_INT 2
#define SDF_BLEND_SMTH_UNION 3
#define SDF_BLEND_SMTH_DIFF 4
#define SDF_BLEND_SMTH_INT 5
#define SDF_BLEND_COUNT 6

#define MATERIAL_COUNT 4

using namespace std;
using namespace glm;

struct Uniforms{
    glm::mat4 IVP;
    glm::vec4 eye;
    glm::vec4 nfwh;
    glm::vec4 seed;
};

struct edit_params{
    vec3 t, r, s;
    int dis_type, blend_type, mat_id;
    float smoothness;
    edit_params(){
        memset(this, 0, sizeof(*this));
        smoothness = 0.5f;
    }
};

class SDF {
    mat4 inv_xform;
    vec4 parameters; // [dis_type, blend_type, smoothness, material_id]
    vec4 extra_params; // [object_id, uv_scale]
public:
    SDF(){}
    SDF(const edit_params& params){
        static int id = 0;
        inv_xform = glm::inverse(
            glm::translate({}, params.t) *
            glm::orientate4(params.r) 
        );
        parameters = vec4(
            float(params.dis_type),
            float(params.blend_type),
            params.smoothness,
            float(params.mat_id)
        );
        extra_params = vec4(
            float(id++),
            1.0f,
            0.0f,
            0.0f
        );
    }
};

class SDF_Edits{
    SDF* sdfs;
    SSBO ssbo;
    int capacity;
    int tail;
public:
    SDF_Edits() : sdfs(nullptr), capacity(0), tail(0){
    }
    ~SDF_Edits(){
        delete[] sdfs;
    }
    void init(int cap, int channel){
        if(sdfs){
            delete[] sdfs;
        }
        capacity = cap;
        tail = 0;
        if(capacity){
            sdfs = new SDF[capacity];
        }
        ssbo.init(sdfs, sizeof(SDF) * tail, channel);
    }
    void add_edit(const edit_params& params){
        assert(sdfs && tail < capacity);
        sdfs[tail++] = SDF(params);
        if(tail >= capacity){
            const int new_capacity = capacity << 1;
            SDF* new_sdfs = new SDF[new_capacity];
            memcpy(new_sdfs, sdfs, sizeof(SDF) * tail);
            delete[] sdfs;
            sdfs = new_sdfs;
            capacity = new_capacity;
        }
    }
    void update_brush(const edit_params& params){
        if(!capacity){
            capacity = 8;
            tail = 1;
            sdfs = new SDF[capacity];
        }
        if(!tail)
            ++tail;
        
        sdfs[0] = SDF(params);
    }
    void undo(){
        if(tail > 0){
            --tail;
        }
    }
    void uniform(ComputeShader& shader){
        shader.setUniformInt("num_sdfs", tail);
        ssbo.upload(sdfs, sizeof(SDF) * tail);
    }
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

inline float absSum(const glm::vec3& v){
    return fabsf(v.x) + fabsf(v.y) + fabsf(v.z);
}

bool v3_equal(const glm::vec3& a, const glm::vec3& b){
    return absSum(a - b) == 0.0f;
}

void editing_behaviour(Input& input, Camera& cam, SDF_Edits& edits){
    static edit_params params;

    params.t = cam.getEye() + cam.getAxis() * 5.0f;

    bool shouldAppend = false;

    for(int* i = input.beginDownKeys(); i != input.endDownKeys(); ++i){
        switch(*i){
            case GLFW_MOUSE_BUTTON_1:
                shouldAppend = true;
            break;
            case GLFW_KEY_1:
                params.dis_type = glm::clamp(params.dis_type - 1, 0, SDF_TYPE_COUNT - 1);
            break;
            case GLFW_KEY_2:
                params.dis_type = glm::clamp(params.dis_type + 1, 0, SDF_TYPE_COUNT - 1);
            break;
            case GLFW_KEY_3:
                params.blend_type = glm::clamp(params.blend_type - 1, 0, SDF_BLEND_COUNT - 1);
            break;
            case GLFW_KEY_4:
                params.blend_type = glm::clamp(params.blend_type + 1, 0, SDF_BLEND_COUNT - 1);
            break;
            case GLFW_KEY_5:
                params.smoothness *= 0.99f;
            break;
            case GLFW_KEY_6:
                params.smoothness *= 1.01f;
            break;
            case GLFW_KEY_UP:
                params.s *= 1.01f;
            break;
            case GLFW_KEY_DOWN:
                params.s *= 0.99f;
            break;
            case GLFW_KEY_LEFT:
                params.mat_id = glm::clamp(params.mat_id - 1, 0, MATERIAL_COUNT - 1);
            break;
            case GLFW_KEY_RIGHT:
                params.mat_id = glm::clamp(params.mat_id + 1, 0, MATERIAL_COUNT - 1);
            break;
            case GLFW_KEY_Z:
                edits.undo();
            break;
        }
    }

    edits.update_brush(params);
    if(shouldAppend){
        edits.add_edit(params);
    }
}

int main(int argc, char* argv[]){
    srand((unsigned)time(NULL));
    int WIDTH = 1280, HEIGHT = 720;
    if(argc == 3){
        WIDTH = atoi(argv[1]);
        HEIGHT = atoi(argv[2]);
    }
    
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
    
    GLProgram color("assets/vert.glsl", "assets/frag.glsl");
    ComputeShader depth("assets/depth.glsl");

    Texture4f colTex;
    colTex.init(WIDTH, HEIGHT);
    colTex.setCSBinding(0, GL_READ_WRITE);

    GLScreen screen;
    Timer timer;
    SDF_Edits edits;

    edits.init(32, 3);
    
    Uniforms uni;
    uni.IVP = camera.getIVP();
    uni.eye = glm::vec4(camera.getEye(), 1.0f);
    uni.nfwh = glm::vec4(camera.getNear(), camera.getFar(), (float)WIDTH, (float)HEIGHT);
    UBO unibuf(&uni, sizeof(uni), 2);

    Texture4uc textures[3];
    textures[0].init("assets/pt_tex_reflection.png");
    textures[1].init("assets/pt_tex_emissive.png");
    textures[2].init("assets/pt_tex_normal.png");

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
            frame = 2.0f;
        
        uni.IVP = camera.getIVP();
        uni.eye = glm::vec4(camera.getEye(), 1.0f);
        uni.seed = glm::vec4(rand() * irm, rand() * irm, rand() * irm, frame);
        unibuf.upload(&uni, sizeof(uni));

        editing_behaviour(input, camera, edits);
        
        depth.bind();
        textures[0].bind(4, "reflectanceTex0", depth);
        textures[1].bind(5, "emittanceTex0", depth);
        textures[2].bind(6, "normalTex0", depth);
        edits.uniform(depth);
        depth.call(callsizeX, callsizeY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        
        color.bind();
        colTex.bind(0, "color", color);
        screen.draw();
        
        window.swap();
        if(frame < 10000000.0f){
            frame++;
        }
    }

    return 0;
}

