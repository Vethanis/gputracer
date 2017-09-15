#include "myglheaders.h"
#include "SSBO.h"
#include "debugmacro.h"

void SSBO::init(void* ptr, size_t bytes, unsigned binding){
    glGenBuffers(1, &id);
    MYGLERRORMACRO
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    MYGLERRORMACRO
    if(ptr && bytes){
        glBufferData(GL_SHADER_STORAGE_BUFFER, bytes, ptr, GL_DYNAMIC_COPY);
        MYGLERRORMACRO
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
    MYGLERRORMACRO
}
SSBO::~SSBO(){
    if(id){
        glDeleteBuffers(1, &id);
        MYGLERRORMACRO
    }
}
void SSBO::upload(void* ptr, size_t bytes){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    MYGLERRORMACRO
    glBufferData(GL_SHADER_STORAGE_BUFFER, bytes, ptr, GL_DYNAMIC_COPY);
    MYGLERRORMACRO
}
