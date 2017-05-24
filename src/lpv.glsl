#version 430 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(binding=2) uniform CAM_BUF
{
    mat4 IVP;
    vec4 eye;
    vec4 nfwh;
    vec4 seed;
    ivec4 lpvsz_frameid;
};

#define EYE eye.xyz
#define NEAR nfwh.x
#define FAR nfwh.y
#define WIDTH nfwh.z
#define HEIGHT nfwh.w
#define SAMPLES seed.w
#define LPV_SIZE lpvsz_frameid.x
#define FRAME_ID lpvsz_frameid.w
#define LPV_RADIUS eye.w

struct Material{
    vec4 reflectance, emittance;
};

layout(binding=3) buffer MAT_BUF{
    Material materials[];
};
#define ROUGHNESS(i) materials[(i)].emittance.w

layout(binding=4) buffer LPV_BUF{
    vec4 lpv_buf[];
};


vec3 toWorld(float x, float y, float z){
    vec4 t = vec4(x, y, z, 1.0);
    t = IVP * t;
    return vec3(t/t.w);
}

struct MapSample{
    float distance;
    int matid;
};

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

// Repeat in three dimensions
vec3 tri(inout vec3 p, vec3 size) {
	vec3 c = floor((p + size*0.5)/size);
	p = mod(p + size*0.5, size) - size*0.5;
	return c;
}

MapSample map(vec3 ray){
    MapSample a = sphere(ray, // light
        vec3(0.0f, 0.0f, 0.0f),
        0.5f,
        0);
    MapSample b = sphere(ray,    // red
        vec3(-1.5f, 0.0f, 0.0f),
        1.0f,
        9);
    a = join(a, b);
    b = sphere(ray,    // green
        vec3(0.0f, 1.5f, 0.0f),
        1.0f,
        10);
    a = join(a, b);
    b = sphere(ray,    // chrome
        vec3(0.0f, 0.0f, 1.5f),
        1.0f,
        8);
    a = join(a, b);
    return a;
}

int getidx(int buf, ivec3 ipos){
    ipos = clamp(ipos, 0, LPV_SIZE);
    int idx = ipos.x + ipos.y * LPV_SIZE + ipos.z * LPV_SIZE * LPV_SIZE;
    idx += LPV_SIZE * LPV_SIZE * LPV_SIZE * (buf & 1);
    return idx;
}

vec3 read(int buf, ivec3 ipos){
    return lpv_buf[getidx(buf, ipos)].xyz;
}

void write(int buf, ivec3 ipos, vec3 val){
    val = clamp(val, 0.0, 1.0);
    lpv_buf[getidx(buf, ipos)].xyz = val;
}

vec3 getneighbors(int buf, ivec3 ipos){
	vec3 sum = vec3(0.0f);
    sum += read(buf, ipos + ivec3(1, 0, 0));
    sum += read(buf, ipos + ivec3(-1, 0, 0));
    sum += read(buf, ipos + ivec3(0, 1, 0));
    sum += read(buf, ipos + ivec3(0, -1, 0));
    sum += read(buf, ipos + ivec3(0, 0, 1));
    sum += read(buf, ipos + ivec3(0, 0, -1));
	return sum * 0.166666;
}

vec3 calc(int buf, vec3 pos, ivec3 ipos){
	MapSample hit = map(pos);
	if(hit.distance < (LPV_RADIUS * 0.25f)) // occluded, no light
		return vec3(0.0f);

	vec3 inc = getneighbors(buf, ipos);

	if(hit.distance < LPV_RADIUS) // reflector
		return inc * materials[hit.matid].reflectance.xyz + vec3(0.1);// materials[hit.matid].emittance.xyz;

	// transmission
	return inc;
}

void main(){
    ivec3 ipos = ivec3(gl_GlobalInvocationID.xyz);
	if(ipos.x >= LPV_SIZE || ipos.y >= LPV_SIZE || ipos.z >= LPV_SIZE)
		return;

	int a = FRAME_ID & 1;
	int b = (FRAME_ID + 1) & 1;

	float hsize = float(LPV_SIZE >> 1);
	vec3 pos = EYE + LPV_RADIUS * (vec3(ipos) - hsize);
	vec3 outgoing = calc(a, pos, ipos);
	write(b, ipos, outgoing);
}
