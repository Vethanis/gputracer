#ifndef TEXTURE_H
#define TEXTURE_H

#include "myglheaders.h"
#include "glm/glm.hpp"
#include "debugmacro.h"
#include "glprogram.h"
#include "ints.h"
#include "image.h"

template<typename T, s32 FullType, s32 Channels, s32 ComponentType>    
struct Texture{    
    u32 width, height;
    u32 handle;
    void init(u32 w, u32 h, bool mip=false){
        width = w;
        height = h;
        glGenTextures(1, &handle);    
        glBindTexture(GL_TEXTURE_2D, handle);    
        if(mip){
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);    MYGLERRORMACRO    
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);    MYGLERRORMACRO    
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);    MYGLERRORMACRO    
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    MYGLERRORMACRO    
        }
        else{
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    MYGLERRORMACRO    
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    MYGLERRORMACRO    
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);    MYGLERRORMACRO    
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    MYGLERRORMACRO    
        }    
        glTexImage2D(GL_TEXTURE_2D, 0, FullType, width, height, 0, Channels, ComponentType, NULL);    MYGLERRORMACRO    
        if(mip)    
            glGenerateMipmap(GL_TEXTURE_2D);    MYGLERRORMACRO    
    
        glBindTexture(GL_TEXTURE_2D, 0);    
        MYGLERRORMACRO    
    }
    void release(){
        glDeleteTextures(1, &handle);    MYGLERRORMACRO  
    }
    void upload(const void* p, bool mip=false){
        if(!p)return;
        
        glBindTexture(GL_TEXTURE_2D, handle);  
        glTexImage2D(GL_TEXTURE_2D, 0, FullType, width, height, 0, Channels, ComponentType, p);   MYGLERRORMACRO     
        if(mip)    
            glGenerateMipmap(GL_TEXTURE_2D);    MYGLERRORMACRO   
    }
    void init(image& img){
        init(img.width, img.height, true);
        upload(img.data);
    }
    Texture(s32 w, s32 h, bool mip=false){ init(w, h, mip); }
    Texture(image& img){ init(img); }
    Texture(const char* filename){ init(image(filename)); }
    Texture(){
        init(1, 1);
    }
    ~Texture(){
        release();   
    }
    void bind(s32 channel, const char* uname, GLProgram& prog){    
        glActiveTexture(GL_TEXTURE0 + channel);    
        MYGLERRORMACRO    
        glBindTexture(GL_TEXTURE_2D, handle);    
        MYGLERRORMACRO    
        prog.setUniformInt(uname, channel);    
    }
    void setCSBinding(s32 binding){    
        glBindImageTexture(0, handle, 0, GL_FALSE, 0, GL_READ_WRITE, FullType);    
        MYGLERRORMACRO    
    } 
};

typedef Texture<f32,       GL_R32F,    GL_RED,  GL_FLOAT> Texture1f;
typedef Texture<glm::vec2, GL_RG32F,   GL_RG,   GL_FLOAT> Texture2f;
typedef Texture<glm::vec3, GL_RGB32F,  GL_RGB,  GL_FLOAT> Texture3f;
typedef Texture<glm::vec4, GL_RGBA32F, GL_RGBA, GL_FLOAT> Texture4f;

typedef Texture<s32,        GL_R32I,    GL_RED,  GL_INT> Texture1i;
typedef Texture<glm::ivec2, GL_RG32I,   GL_RG,   GL_INT> Texture2i;
typedef Texture<glm::ivec3, GL_RGB32I,  GL_RGB,  GL_INT> Texture3i;
typedef Texture<glm::ivec4, GL_RGBA32I, GL_RGBA, GL_INT> Texture4i;

typedef Texture<u32,        GL_R32UI,    GL_RED,  GL_UNSIGNED_INT> Texture1u;
typedef Texture<glm::uvec2, GL_RG32UI,   GL_RG,   GL_UNSIGNED_INT> Texture2u;
typedef Texture<glm::uvec3, GL_RGB32UI,  GL_RGB,  GL_UNSIGNED_INT> Texture3u;
typedef Texture<glm::uvec4, GL_RGBA32UI, GL_RGBA, GL_UNSIGNED_INT> Texture4u;

struct ucvec2{u8 x, y;};
struct ucvec3{u8 x, y, z;};
struct ucvec4{u8 x, y, z, w;};

typedef Texture<u8,     GL_R8UI,    GL_RED,  GL_UNSIGNED_BYTE> Texture1uc;
typedef Texture<ucvec2, GL_RG8UI,   GL_RG,   GL_UNSIGNED_BYTE> Texture2uc;
typedef Texture<ucvec3, GL_RGB8UI,  GL_RGB,  GL_UNSIGNED_BYTE> Texture3uc;
typedef Texture<ucvec4, GL_RGBA8UI, GL_RGBA, GL_UNSIGNED_BYTE> Texture4uc;

#endif

