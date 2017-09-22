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
    s32 width, height;
    u32 handle;
    void init(s32 w, s32 h, bool mip = false){
        width = w;
        height = h;
        glGenTextures(1, &handle);    MYGLERRORMACRO
        glBindTexture(GL_TEXTURE_2D, handle);    MYGLERRORMACRO
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
            glGenerateMipmap(GL_TEXTURE_2D); 
            
        MYGLERRORMACRO
    
    }
    void release(){
        if(!handle)
            return;
        glDeleteTextures(1, &handle);    MYGLERRORMACRO  
    }
    void upload(const void* p, bool mip=false){
        if(!p)return;
        
        glBindTexture(GL_TEXTURE_2D, handle);  MYGLERRORMACRO
        glTexImage2D(GL_TEXTURE_2D, 0, FullType, width, height, 0, Channels, ComponentType, p);   MYGLERRORMACRO     
        if(mip)    
            glGenerateMipmap(GL_TEXTURE_2D);    
            
        MYGLERRORMACRO   
    }
    void init(const image& img){
        init(img.width, img.height, true);
        upload(img.data);
    }
    void init(const char* filename){
        init(image(filename));
    }
    Texture() : handle(0){
    }
    ~Texture(){
        release();   
    }
    void bind(s32 channel, const char* uname, GLProgram& prog){    
        glActiveTexture(GL_TEXTURE0 + channel);    
        MYGLERRORMACRO    
        glBindTexture(GL_TEXTURE_2D, handle);    
        MYGLERRORMACRO
        glBindImageTexture(channel, handle, 0, GL_FALSE, 0, GL_READ_WRITE, FullType); 
        MYGLERRORMACRO 
        prog.setUniformInt(uname, channel);    
    }
    void bind(s32 channel, const char* uname, ComputeShader& prog){    
        glActiveTexture(GL_TEXTURE0 + channel);    
        MYGLERRORMACRO    
        glBindTexture(GL_TEXTURE_2D, handle);    
        MYGLERRORMACRO
        prog.setUniformInt(uname, channel);
    }
    void setCSBinding(int binding, int mode=GL_READ_ONLY){
        glBindImageTexture(binding, handle, 0, GL_FALSE, 0, mode, FullType);
        MYGLERRORMACRO 
    }
};

typedef Texture<f32,       GL_R32F,    GL_RED,  GL_FLOAT> Texture1f;
typedef Texture<glm::vec2, GL_RG32F,   GL_RG,   GL_FLOAT> Texture2f;
typedef Texture<glm::vec4, GL_RGBA32F, GL_RGBA, GL_FLOAT> Texture4f;

struct ucvec2{u8 x, y;};
struct ucvec4{u8 x, y, z, w;};

typedef Texture<u8,     GL_R8,    GL_RED,  GL_UNSIGNED_BYTE> Texture1uc;
typedef Texture<ucvec2, GL_RG8,   GL_RG,   GL_UNSIGNED_BYTE> Texture2uc;
typedef Texture<ucvec4, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE> Texture4uc;

#endif

