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

struct SDF{
    vec4 position; // w sdf type
    vec4 scale; // w blend type
    vec4 rotation; // quaternion
    vec4 parameters; 
    // x -> object id 
    // y -> material id 
    // z -> blend smoothness
    // w -> uv scale
};

layout(binding=3) buffer SDF_BUF{   
    SDF sdfs[];
};

uniform int num_sdfs;

#define sdf_position(i) sdfs[i].position.xyz
#define sdf_rotation(i) sdfs[i].rotation.xyzw
#define sdf_scale(i) sdfs[i].scale.xyz
#define sdf_radius(i) sdfs[i].scale.x
#define sdf_direction(i) sdfs[i].rotation.xyz
#define sdf_distance_type(i) int(sdfs[i].position.w)
#define sdf_blend_type(i) int(sdfs[i].scale.w)
#define sdf_blend_smoothness(i) sdfs[i].parameters.z
#define sdf_object_id(i) int(sdfs[i].parameters.x)
#define sdf_material_id(i) int(sdfs[i].parameters.y)
#define sdf_roughness(refl) refl.w
#define sdf_uv_scale(i) sdfs[i].parameters.w

layout(binding=8 ) uniform sampler2D reflectanceTex0;
layout(binding=9 ) uniform sampler2D reflectanceTex1;
layout(binding=10) uniform sampler2D reflectanceTex2;
layout(binding=11) uniform sampler2D reflectanceTex3;

layout(binding=12) uniform sampler2D emittanceTex0;
layout(binding=13) uniform sampler2D emittanceTex1;
layout(binding=14) uniform sampler2D emittanceTex2;
layout(binding=15) uniform sampler2D emittanceTex3;

layout(binding=16) uniform sampler2D normalTex0;
layout(binding=17) uniform sampler2D normalTex1;
layout(binding=18) uniform sampler2D normalTex2;
layout(binding=19) uniform sampler2D normalTex3;

vec4 sdf_reflectance_texture(int i, vec2 uv){
    int mat_id = sdf_material_id(i);
    switch(mat_id){
        case 0: return texture(reflectanceTex0, uv);
        case 1: return texture(reflectanceTex1, uv);
        case 2: return texture(reflectanceTex2, uv);
        case 3: return texture(reflectanceTex4, uv);
    }
    return vec4(0.0);
}

vec4 sdf_emittance_texture(int i, vec2 uv){
    int mat_id = sdf_material_id(i);
    switch(mat_id){
        case 0: return texture(emittanceTex0, uv);
        case 1: return texture(emittanceTex1, uv);
        case 2: return texture(emittanceTex2, uv);
        case 3: return texture(emittanceTex4, uv);
    }
    return vec4(0.0);
}

vec3 sdf_normal_texture(int i, vec2 uv){
    int mat_id = sdf_material_id(i);
    switch(mat_id){
        case 0: return texture(normalTex0, uv).rgb;
        case 1: return texture(normalTex1, uv).rgb;
        case 2: return texture(normalTex2, uv).rgb;
        case 3: return texture(normalTex4, uv).rgb;
    }
    return vec3(0.0, 0.0, -1.0);
}

float vmax(vec3 a){
    return max(max(a.x, a.y), a.z);
}

vec2 sdf_sphere(int id, vec3 ray){
    return vec2(length(ray) - sdf_radius(id), float(id));
}

vec2 sdf_box(int id, vec3 ray){
    vec3 d = abs(ray) - sdf_scale(id);
    return vec2(vmax(d), id);
}

vec2 sdf_plane(int id, vec3 ray){
    return vec2(dot(ray, sdf_direction(id)), float(id));
}

vec2 sdf_distance(int i, vec3 point){
    // transform point into unit sphere space
    point = point - sdf_position(i);
    point = quat_inv_rotate(point, sdf_rotation(i));
    point = point / sdf_scale(i);

    // use unit-sphere sdfs
    switch(sdf_distance_type(i)){
        case 0: return sdf_sphere(i, point);
        case 1: return sdf_box(i, point);
        case 2: return sdf_plane(i, point);
        case 3: return sdf_cone(i, point);
        case 4: return sdf_pyramid(i, point);
        case 5: return sdf_torus(i, point);
        case 6: return sdf_cylinder(i, point);
        case 7: return sdf_capsule(i, point);
        case 8: return sdf_disk(i, point);
    }
    
    return vec2(100000.0, 0.0);
}

vec2 sdf_blend_union(vec2 a, vec2 b){
    return a.x < b.x ? a : b;
}

vec2 sdf_blend_difference(vec2 a, vec2 b){
    b.x = -b.x;
    return a.x > b.x ? a : b;
}

vec2 sdf_blend_intersect(vec2 a, vec2 b){
    return a.x > b.x ? a : b;
}

vec2 sdf_blend_smooth_union(vec2 a, vec2 b, float k){
    float e = max(k - abs(a.x - b.x), 0.0);
    float dis = min(a.x, b.x) - e * e * 0.25 / k;
    return vec2(dis, a.x < b.x ? a.y : b.y);
}

vec2 sdf_blend_smooth_difference(vec2 a, vec2 b, float k){
    a.x = -a.x;
    vec2 r = sdf_blend_smooth_union(a, b, k);
    r.x = -r.x;
    return r;
}

vec2 sdf_blend_smooth_intersect(vec2 a, vec2 b, float k){
    float e = max(k - abs(a.x - b.x), 0.0);
    float dis = max(a.x, b.x) - e * e * 0.25 / k;
    return vec2(dis, a.x > b.x ? a.y : b.y);
}

vec2 sdf_blend(int i, vec2 a, vec2 b){
    switch(sdf_blend_type(i)){
        case 0: return sdf_blend_union(a, b);
        case 1: return sdf_blend_difference(a, b);
        case 2: return sdf_blend_intersect(a, b);
        case 3: return sdf_blend_smooth_union(a, b, sdf_blend_smoothness(i));
        case 4: return sdf_blend_smooth_difference(a, b, sdf_blend_smoothness(i));
        case 5: return sdf_blend_smooth_intersect(a, b, sdf_blend_smoothness(i));
    }
    return sdf_blend_union(a, b);
}

vec2 sdf_map(vec3 ray){
    vec2 sam;
    sam.x = 100000.0;
    sam.y = -1.0;
    for(int i = 0; i < num_sdfs; ++i){
        sam = sdf_blend(i, sdf_distance(i, ray), x);
    }
    return sam;
}

vec3 sdf_map_normal(vec3 point){
    vec3 e = vec3(0.0001, 0.0, 0.0);
    return normalize(vec3(
        map(point + e.xyz).x - map(point - e.xyz).x,
        map(point + e.zxy).x - map(point - e.zxy).x,
        map(point + e.zyx).x - map(point - e.zyx).x
    ));
}

vec3 toWorld(float x, float y, float z){
    vec4 t = vec4(x, y, z, 1.0);
    t = IVP * t;
    return vec3(t/t.w);
}

float randUni(inout uint f){
    f = (f ^ 61) ^ (f >> 16);
    f *= 9;
    f = f ^ (f >> 4);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15);
    return fract(float(f) * 2.3283064e-10);
}

float rand( inout uint f) {
   return randUni(f) * 2.0 - 1.0;
}

vec3 uniHemi(vec3 N, inout uint s){
    vec3 dir;
    float len;
    int i = 0;
    do{
        dir = vec3(rand(s), rand(s), rand(s));
        len = length(dir);
    }while(len > 1.0 && i < 5);
    
    if(dot(dir, N) < 0.0)
        dir *= -1.0;
    
    return dir / len;
}

vec3 cosHemi(vec3 N, inout uint s){
    // derived from smallpt
    
    float r1 = 3.141592 * 2.0 * randUni(s);
    float r2 = randUni(s);
    float r2s = sqrt(r2);
    
    vec3 u;
    if(abs(N.x) > 0.1)
        u = cross(vec3(0.0, 1.0, 0.0), N);
    else
        u = cross(vec3(1.0, 0.0, 0.0), N);
    
    u = normalize(u);
    vec3 v = cross(N, u);
    return normalize(
        u * cos(r1) * r2s 
        + v * sin(r1) * r2s 
        + N * sqrt(1.0 - r2)
        );
}

vec3 roughBlend(vec3 newdir, vec3 I, vec3 N, float roughness){
    return normalize(
        mix(
            normalize(
                reflect(I, N)), 
            newdir, 
            roughness
            )
        );
}

float absum(vec3 a){
    return abs(a.x) + abs(a.y) + abs(a.z);
}

vec2 uv_from_ray(vec3 N, vec3 p){
    vec3 d = abs(N);
    float a = vmax(d);
    if(a == d.x){
        return vec2(p.z, p.y);
    }
    else if(a == d.y){
        return vec2(p.x, p.z);
    }
    return vec2(p.x, p.y);
}

vec3 trace(vec3 rd, vec3 eye, inout uint s){
    float e = 0.001;
    vec3 col = vec3(0.0, 0.0, 0.0);
    vec3 mask = vec3(1.0, 1.0, 1.0);
    
    for(int i = 0; i < 3; i++){    // bounces
        vec2 sam;
        
        for(int j = 0; j < 45; j++){ // steps
            sam = sdf_map(eye);
            if(abs(sam.distance) < e){
                break;
            }
            eye = eye + rd * sam.x;
        }
        
        vec3 N = map_normal(eye);

        const int sdf_id = int(sam.y);
        if(sdf_id < 0 || sdf_id >= num_sdfs)
            break;

        const vec2 uv = uv_from_ray(N, eye) * sdf_uv_scale(sdf_id);
        const vec4 refl = sdf_reflectance_texture(sdf_id, uv);
        const vec4 emit = sdf_emittance_texture(sdf_id, uv);
        const vec3 tN = sdf_normal_texture(sdf_id, uv);
        
        {   // update direction
            const vec3 I = rd;
            rd = uniHemi(N, s);
            rd = roughBlend(rd, I, N, sdf_roughness(refl));
            eye += N * e * 10.0f;
        }
        
        col += mask * emit.rgb;
        mask *= 2.0 * refl.rgb * abs(dot(N, rd));
    }
    
    return col;
}

void main(){
    const ivec2 pix = ivec2(gl_GlobalInvocationID.xy);  
    const ivec2 size = imageSize(color);
    if (pix.x >= size.x || pix.y >= size.y) return;
    
    uint s = uint(seed.z + 10000.0 * dot(seed.xy, gl_GlobalInvocationID.xy));
    const vec2 aa = vec2(rand(s), rand(s)) * 0.5;
    const vec2 uv = (vec2(pix + aa) / vec2(size))* 2.0 - 1.0;
    const vec3 rd = normalize(toWorld(uv.x, uv.y, 0.0) - EYE);
    
    vec3 col = clamp(trace(rd, EYE, s), vec3(0.0), vec3(1.0));
    const vec3 oldcol = imageLoad(color, pix).rgb;
    
    col = mix(oldcol, col, 1.0 / SAMPLES);
    
    imageStore(color, pix, vec4(col, 1.0));
}
