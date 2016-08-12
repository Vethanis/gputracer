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
    return fract(float(f) * 0.00000001) * 2.0 - 1.0;
}

vec3 randomDir(vec3 N, vec3 rd, float roughness, inout uint s){
    vec3 dir, ref = normalize(reflect(rd, N));
    int i = 0;
    do{
        dir.x = rand(s);
        dir.y = rand(s);
        dir.z = rand(s);
        i++;
    }while(dot(dir, N) <= 0.0 && i < 20);
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
    MapSample a = sphere(ray, // light sphere
        vec3(0.0f, -1.0f, 3.0f),
        1.0f,
        4);
    a = join(a, sphere(ray, // chrome sphere
        vec3(0.0f, -3.0f, 0.0f),
        1.0f,
        1));
    a = join(a, sphere(ray, // blue sphere
        vec3(-0.5f, -3.5f, 3.0f),
        0.5f,
        2));
    a = join(a, sphere(ray, // red sphere
        vec3(0.5f, -3.5f, 3.0f),
        0.5f,
        3));
    a = join(a, box(ray, // box above spheres
        vec3(0.0f, -2.5f, 3.0f),
        vec3(1.2f, 0.1f, 1.0f),
        0));
    a = join(a, box(ray, // box left of spheres
        vec3(-1.2f, -3.5f, 3.0f),
        vec3(0.1f, 1.0f, 1.0f),
        0));
    a = join(a, box(ray, // box right of spheres
        vec3(1.2f, -3.5f, 3.0f),
        vec3(0.1f, 1.0f, 1.0f),
        0));
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

vec3 trace(vec3 rd, vec3 eye, inout uint s){
    float e = 0.0001;
	vec3 col = vec3(0.0, 0.0, 0.0);
    vec3 mask = vec3(1.0, 1.0, 1.0);

    int depth = 3 + int( SAMPLES * 0.005);
    depth = min(depth, 8);
    
    for(int i = 0; i < depth; i++){    // bounces
        MapSample sam;
        bool hit = false;
        
        for(int j = 0; j < 200; j++){ // steps
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
    
    uint s = uint(seed.z + 10000.0 * dot(seed.xy, gl_GlobalInvocationID.xy));
    vec2 aa = vec2(rand(s), rand(s)) * 0.5;
	vec2 uv = (vec2(pix + aa) / vec2(size))* 2.0 - 1.0;
	vec3 rd = normalize(toWorld(uv.x, uv.y, 1.0) - EYE);
    
	vec3 col = trace(rd, EYE, s);
    vec3 oldcol = imageLoad(color, pix).rgb;
    
    float a = 1.0 / SAMPLES;
    float b = 1.0 - a;
    
    col = a * col + b * oldcol;
    
	imageStore(color, pix, vec4(col, 1.0));
}

