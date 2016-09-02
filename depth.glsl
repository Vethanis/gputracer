#version 430 core

layout(local_size_x = 8, local_size_y = 8) in;

layout(binding = 0, rgba32f) uniform image2D color;

layout(binding=2) uniform CAM_BUF
{
    mat4 IVP;
    vec4 eye;
    vec4 nfwh;
    vec4 seed;
};

#define EYE eye.xyz
#define NEAR nfwh.x
#define FAR nfwh.y
#define WIDTH nfwh.z
#define HEIGHT nfwh.w
#define SAMPLES seed.w

struct Material{
    vec4 reflectance, emittance;
};

layout(binding=3) buffer SDF_BUF{   
    Material materials[16];
};

#define ROUGHNESS(i) materials[(i)].emittance.w

vec3 toWorld(float x, float y, float z){
    vec4 t = vec4(x, y, z, 1.0);
    t = IVP * t;
    return vec3(t/t.w);
}

struct MapSample{
    float distance;
    int matid;
};

float rand( inout uint f) {
    f = (f ^ 61) ^ (f >> 16);
    f *= 9;
    f = f ^ (f >> 4);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15);
    return fract(float(f) * 2.3283064e-10) * 2.0 - 1.0;
}

float randUni(inout uint f){
    f = (f ^ 61) ^ (f >> 16);
    f *= 9;
    f = f ^ (f >> 4);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15);
    return fract(float(f) * 2.3283064e-10);
}

vec3 randomDir(vec3 N, vec3 rd, float roughness, inout uint s){
    vec3 dir;
    int i = 0;
    do{
        dir = vec3(rand(s), rand(s), rand(s));
    }while(length(dir) > 1.0 && i < 10);
    
    if(dot(dir, N) < 0.0)
        dir *= -1.0;
    dir = normalize(dir);
    vec3 ref = normalize(reflect(rd, N));
    
    return normalize(mix(ref, dir, roughness));
}

float vmax(vec3 a){
    return max(max(a.x, a.y), a.z);
}

MapSample sphere(vec3 ray, vec3 location, float radius, int mat){
    return MapSample(distance(ray, location) - radius, mat);
}

MapSample box(vec3 ray, vec3 location, vec3 dimension, int mat){
    vec3 d = abs(ray - location) - dimension;
    return MapSample(vmax(d), mat);
}

MapSample plane(vec3 ray, vec3 location, vec3 normal, int mat){
    return MapSample(dot(ray - location, normal), mat);
}

MapSample join(MapSample a, MapSample b){
    if(a.distance <= b.distance)
        return a;
    return b;
}

float diff(MapSample a, MapSample b){
    return a.distance - b.distance;
}

vec3 tri(vec3 r, float d){
    return vec3(
        modf(r.x, d),
        modf(r.y, d),
        modf(r.z, d)
    );
}

/*
 0: white
 1: mirror
 2: blue
 3: red
 4: light
*/

MapSample map(vec3 ray){
    MapSample a = sphere(ray, // white light
        vec3(4.0f, 4.0f, 4.0f),
        1.0f,
        0);
    a = join(a, sphere(ray, // light
        vec3(-4.0f, 4.0f, 4.0f),
        1.0f,
        1));
    a = join(a, sphere(ray, // light
        vec3(-4.0f, 4.0f, -4.0f),
        1.0f,
        2));
    a = join(a, sphere(ray, // light
        vec3(4.0f, 4.0f, -4.0f),
        1.0f,
        3));
    a = join(a, sphere(ray, // chrome spheres
        vec3(-4.0f, 0.0f, 0.0f),
        1.0f,
        8));
    a = join(a, sphere(ray, // chrome spheres
        vec3(-2.0f, 0.0f, 0.0f),
        1.0f,
        7));
    a = join(a, sphere(ray, // chrome spheres
        vec3(0.0f, 0.0f, 0.0f),
        1.0f,
        6));
    a = join(a, sphere(ray, // chrome spheres
        vec3(2.0f, 0.0f, 0.0f),
        1.0f,
        5));
    a = join(a, sphere(ray, // chrome spheres
        vec3(4.0f, 0.0f, 0.0f),
        1.0f,
        4));
    a = join(a, plane(ray, // left wall
        vec3(-5.0f, 0.0f, 0.0f),
        vec3(1.0f, 0.0f, 0.0f),
        9));
    a = join(a, plane(ray, // right wall
        vec3(5.0f, 0.0f, 0.0f),
        vec3(-1.0f, 0.0f, 0.0f),
        10));
    a = join(a, plane(ray, // ceiling
        vec3(0.0f, 5.0f, 0.0f),
        vec3(0.0f, -1.0f, 0.0f),
        13));
    a = join(a, plane(ray, // floor
        vec3(0.0f, -5.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f),
        12));
    a = join(a, plane(ray, // back
        vec3(0.0f, 0.0f, -5.0f),
        vec3(0.0f, 0.0f, 1.0f),
        11));
    a = join(a, plane(ray, // front
        vec3(0.0f, 0.0f, 11.0f),
        vec3(0.0f, 0.0f, -1.0f),
        14));
    return a;
}

vec3 map_normal(vec3 point){
    vec3 e = vec3(0.0001, 0.0, 0.0);
    return normalize(vec3(
        diff(map(point + e.xyz), map(point - e.xyz)),
        diff(map(point + e.zxy), map(point - e.zxy)),
        diff(map(point + e.zyx), map(point - e.zyx))
    ));
}

vec3 trace(vec3 rd, vec3 eye, inout uint s){
    float e = 0.001;
    vec3 col = vec3(0.0, 0.0, 0.0);
    vec3 mask = vec3(1.0, 1.0, 1.0);
    
    for(int i = 0; i < 5; i++){    // bounces
        MapSample sam;
        
        for(int j = 0; j < 60; j++){ // steps
            sam = map(eye);
            if(abs(sam.distance) < e){
                break;
            }
            eye = eye + rd * sam.distance;
        }
        
        vec3 N = map_normal(eye);
        rd = randomDir(N, rd, ROUGHNESS(sam.matid), s);
        eye += N * e * 10.0f;
        
        col += mask * materials[sam.matid].emittance.rgb;
        mask *= 2.0 * materials[sam.matid].reflectance.rgb * abs(dot(N, rd));
        
        if((abs(mask.x) + abs(mask.y) + abs(mask.z)) < e)
            break;
    }
    
    return col;
}

void main(){
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);  
    ivec2 size = imageSize(color);
    if (pix.x >= size.x || pix.y >= size.y) return;
    
    uint s = uint(seed.z + 10000.0 * dot(seed.xy, gl_GlobalInvocationID.xy));
    vec2 aa = vec2(rand(s), rand(s)) * 0.5;
    vec2 uv = (vec2(pix + aa) / vec2(size))* 2.0 - 1.0;
    vec3 rd = normalize(toWorld(uv.x, uv.y, 0.0) - EYE);
    
    vec3 col = clamp(trace(rd, EYE, s), vec3(0.0), vec3(1.0));
    vec3 oldcol = imageLoad(color, pix).rgb;
    
    col = mix(oldcol, col, 1.0 / SAMPLES);
    
    imageStore(color, pix, vec4(col, 1.0));
}

