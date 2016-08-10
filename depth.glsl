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
    Material materials[5];
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
    return fract(sin(float(f)));
}

vec3 randomDir(vec3 N, vec3 rd, float roughness, inout uint s){
    vec3 dir, ref = normalize(reflect(rd, N));
    int i = 0;
    do{
        dir.x = rand(s) * 2.0 - 1.0;
        dir.y = rand(s) * 2.0 - 1.0;
        dir.z = rand(s) * 2.0 - 1.0;
        i++;
    }while(dot(dir, N) < 0.0f && i < 10);
    return normalize(mix(ref, normalize(dir), roughness));
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

MapSample join(MapSample a, MapSample b){
    if(a.distance <= b.distance)
        return a;
    return b;
}

float diff(MapSample a, MapSample b){
    return a.distance - b.distance;
}

MapSample map(vec3 ray){
    MapSample a = sphere(ray, vec3(1.0f, -2.0f, 0.0f), 1.0f, 1);
    a = join(a, sphere(ray, vec3(-1.0f, -2.0f, 0.0f), 1.0f, 2));
    a = join(a, box(ray,
        vec3(0.0f, -3.0f, 0.0f),
        vec3(1.0f),
        3));
    a = join(a, box(ray,
        vec3(0.0f, 4.0f, 0.0f),
        vec3(2.0f, 0.1f, 2.0f),
        4));
    a = join(a, box(ray, // left wall
        vec3(-4.0f, 0.0f, 0.0f),
        vec3(0.01f, 4.0f, 4.0f),
        0));
    a = join(a, box(ray, // right wall
        vec3(4.0f, 0.0f, 0.0f),
        vec3(0.01f, 4.0f, 4.0f),
        0));
    a = join(a, box(ray, // ceiling
        vec3(0.0f, 4.0f, 0.0f),
        vec3(4.0f, 0.01f, 4.0f),
        0));
    a = join(a, box(ray, // floor
        vec3(0.0f, -4.0f, 0.0f),
        vec3(4.0f, 0.01f, 4.0f),
        0));
    a = join(a, box(ray, // back
        vec3(0.0f, 0.0f, -4.0f),
        vec3(4.0f, 4.0f, 0.01f),
        0));
    a = join(a, box(ray, // front
        vec3(0.0f, 0.0f, 4.0f),
        vec3(4.0f, 4.0f, 0.01f),
        0));
    return a;
}

vec3 map_normal(vec3 point){
    vec3 e = vec3(0.00001, 0.0, 0.0);
    return normalize(vec3(
        diff(map(point + e.xyz), map(point - e.xyz)),
        diff(map(point + e.zxy), map(point - e.zxy)),
        diff(map(point + e.zyx), map(point - e.zyx))
    ));
}

vec3 trace(vec3 rd, vec3 eye){
    float e = 0.0001;
	vec3 col = vec3(0.0, 0.0, 0.0);
    vec3 mask = vec3(1.0, 1.0, 1.0);
    uint s = uint(1024.0 * dot(seed.xy, gl_GlobalInvocationID.xy));
    
    for(int i = 0; i < 10; i++){    // bounces
        MapSample sam;
        bool hit = false;
        
        for(int j = 0; j < 100; j++){ // steps
            sam = map(eye);
            if(abs(sam.distance) < e){
                hit = true;
                break;
            }
            eye = eye + rd * abs(sam.distance);
        }
        
        if(!hit)break;
        
        vec3 N = map_normal(eye);
        rd = randomDir(N, rd, ROUGHNESS(sam.matid), s);
        eye += N * e * 10.0f;
        
        col += mask * materials[sam.matid].emittance.rgb;
        mask *= 2.0 * materials[sam.matid].reflectance.rgb * dot(N, rd);
        
        if((mask.x + mask.y + mask.z) <= 0.0)
            break;
    }
    
    return col;
}

void main(){
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);  
	ivec2 size = imageSize(color);
	if (pix.x >= size.x || pix.y >= size.y) return;

	vec2 uv = (vec2(pix) / vec2(size))* 2.0 - 1.0;
	vec3 rd = normalize(toWorld(uv.x, uv.y, 1.0) - EYE);
    
	vec3 col = trace(rd, EYE); // trace a new ray
    col = pow(col, vec3(1.0 / 2.2));
    
    float a = 1.0 / SAMPLES;
    float b = 1.0 - a;
    
    vec3 oldcol = imageLoad(color, pix).rgb;
    
    col = a * col + b * oldcol;
    
	imageStore(color, pix, vec4(col, 1.0));
}

