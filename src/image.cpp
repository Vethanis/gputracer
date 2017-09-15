#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void image::load(const char* filename){
    int comps = 0;
    data = stbi_load(filename, &width, &height, &comps, 4);
}