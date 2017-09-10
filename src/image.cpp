#include "image.h"
#include "lodepng.h"

void image::load(const char* filename){
    unsigned error = lodepng_decode32_file(&data, &width, &height, filename);
    if(error) { printf("error %u: %s\n", error, lodepng_error_text(error)); }
}