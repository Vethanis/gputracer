#pragma once

#include "ints.h"

struct image{
    u8* data;
    u32 width, height;
    image(){ memset(this, 0, sizeof(image)); }
    ~image(){ free(data); }
    void load(const char* filename);
    image(const char* filename){ load(filename); }
};
