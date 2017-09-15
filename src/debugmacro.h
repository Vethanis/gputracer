#ifndef DEBUGMACRO_H
#define DEBUGMACRO_H

#if 1

#include "myglheaders.h"
#include "stdio.h"

#define MYGLERRORMACRO {    \
    GLenum err = glGetError(); \
    while(err != GL_NO_ERROR){ \
        switch(err){    \
            case 0x0500:    \
                printf("invalid enumeration at ");    \
                PRINTLINEMACRO    \
                assert(false); \
                break;    \
            case 0x501:    \
                printf("invalid value at ");    \
                PRINTLINEMACRO    \
                assert(false); \
                break;     \
            case 0x502: \
                printf("invalid operation at ");    \
                PRINTLINEMACRO    \
                assert(false); \
                break;    \
            case 0x503: \
                printf("stack overflow at ");    \
                PRINTLINEMACRO    \
                assert(false); \
                break;    \
            case 0x504: \
                printf("stack underflow at ");    \
                PRINTLINEMACRO    \
                assert(false); \
                break;    \
            case 0x505: \
                printf("out of memory at ");    \
                PRINTLINEMACRO    \
                assert(false); \
                break;    \
            case 0x506: \
                printf("invalid framebuffer operation at ");    \
                PRINTLINEMACRO    \
                assert(false); \
                break;    \
            case 0x507: \
                printf("context lost at ");    \
                PRINTLINEMACRO    \
                assert(false); \
                break;    \
            case 0x508: \
                printf("table too large at ");    \
                PRINTLINEMACRO    \
                assert(false); \
                break;    \
        }    \
        err = glGetError(); \
    }    \
}

#define PRINTLINEMACRO {    \
    printf("%d in %s\n", __LINE__, __FILE__); \
}    

#else
#define MYGLERRORMACRO ;
#define PRINTLINEMACRO ;
#endif //ifdef DEBUG

#include "glm/glm.hpp"

inline void print(const glm::vec2& v){
    printf("(%.3f, %.3f)\n", v.x, v.y);
}

inline void print(const glm::vec3& v){
    printf("(%.3f, %.3f, %.3f)\n", v.x, v.y, v.z);
}

inline void print(const glm::vec4& v){
    printf("(%.3f, %.3f, %.3f, %.3f)\n", v.x, v.y, v.z, v.w);
}

inline void print(const glm::mat3& m){
    print(m[0]);
    print(m[1]);
    print(m[2]);
}

inline void print(const glm::mat4& m){
    print(m[0]);
    print(m[1]);
    print(m[2]);
    print(m[3]);
}

#endif //debugmacro_h
