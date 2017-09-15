#include "myglheaders.h"
#include "compute_shader.h"
#include "debugmacro.h"
#include <fstream>
#include <vector>
#include <algorithm>
#include "glm/gtc/type_ptr.hpp"

ComputeShader::ComputeShader(const std::string& filename){
    progid = glCreateProgram();
    MYGLERRORMACRO
    shaderid = glCreateShader(GL_COMPUTE_SHADER);
    MYGLERRORMACRO
    
    std::string source;
    std::ifstream stream(filename);
    if(!stream.is_open()){
        printf("Could not open compute shader source %s\n", filename.c_str());
        return;
    }
    
    std::string line;
    while(getline(stream, line))
        source += line + "\n";
    
    char const* src_pointer = source.c_str();
    glShaderSource(shaderid, 1, &src_pointer, NULL);
    MYGLERRORMACRO
    
    GLint result = GL_FALSE;
    int infoLogLength;
    glCompileShader(shaderid);
    MYGLERRORMACRO
    glGetShaderiv(shaderid, GL_COMPILE_STATUS, &result);
    MYGLERRORMACRO
    if(!result){
        glGetShaderiv(shaderid, GL_INFO_LOG_LENGTH, &infoLogLength);
        MYGLERRORMACRO
        std::vector<char> shaderErrorMessage( std::max( infoLogLength, int(1) ) );
        glGetShaderInfoLog(shaderid, infoLogLength, NULL, &shaderErrorMessage[0]);
        MYGLERRORMACRO
        fprintf(stdout, "%s\n", &shaderErrorMessage[0]);
        glDeleteShader(shaderid);
        MYGLERRORMACRO
        glDeleteProgram(progid);
        MYGLERRORMACRO
        return;
    }
    
    glAttachShader(progid, shaderid);
    MYGLERRORMACRO
    glLinkProgram(progid);
    MYGLERRORMACRO
    glDeleteShader(shaderid);
    MYGLERRORMACRO
    
    result = GL_FALSE;
    glGetProgramiv(progid, GL_LINK_STATUS, &result);
    MYGLERRORMACRO
    if(!result){
        glGetProgramiv(progid, GL_INFO_LOG_LENGTH, &infoLogLength);
        MYGLERRORMACRO
        std::vector<char> shaderErrorMessage( std::max( infoLogLength, int(1) ) );
        glGetProgramInfoLog(progid, infoLogLength, NULL, &shaderErrorMessage[0]);
        MYGLERRORMACRO
        fprintf(stdout, "%s\n", &shaderErrorMessage[0]);
        glDeleteProgram(progid);
        MYGLERRORMACRO
        return;
    }
    
    MYGLERRORMACRO
}
ComputeShader::~ComputeShader(){
    glDeleteProgram(progid);
    MYGLERRORMACRO
}
void ComputeShader::bind(){
    glUseProgram(progid);
    MYGLERRORMACRO
}
void ComputeShader::call(unsigned x, unsigned y, unsigned z){
    glDispatchCompute(x, y, z);
    MYGLERRORMACRO
}

int ComputeShader::getUniformLocation(const std::string& name){
    auto iter = uniforms.find(name);
    if(iter == end(uniforms)){
        const int v = glGetUniformLocation(progid, name.c_str());
        MYGLERRORMACRO
        uniforms[name] = v;
        return v;
    }
    return iter->second;
}

void ComputeShader::setUniform(const std::string& name, const glm::vec2& v){
    const int location = getUniformLocation(name);
    if (location == -1)
        return;
    glUniform2fv(location, 1, glm::value_ptr(v));
    MYGLERRORMACRO
}
void ComputeShader::setUniform(const std::string& name, const glm::vec3& v){
    const int location = getUniformLocation(name);
    if (location == -1)
        return;
    glUniform3fv(location, 1, glm::value_ptr(v));
    MYGLERRORMACRO
}
void ComputeShader::setUniform(const std::string& name, const glm::vec4& v){
    const int location = getUniformLocation(name);
    if (location == -1)
        return;
    glUniform4fv(location, 1, glm::value_ptr(v));
    MYGLERRORMACRO
}
void ComputeShader::setUniform(const std::string& name, const glm::mat3& v){
    const int location = getUniformLocation(name);
    if (location == -1)
        return;
    glUniformMatrix3fv(location, 1, false, glm::value_ptr(v));
    MYGLERRORMACRO
}
void ComputeShader::setUniform(const std::string& name, const glm::mat4& v){
    const int location = getUniformLocation(name);
    if (location == -1)
        return;
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(v));
    MYGLERRORMACRO
}
void ComputeShader::setUniformInt(const std::string& name, const int v){
    const int location = getUniformLocation(name);
    if (location == -1)
        return;
    glUniform1i(location, v);
    MYGLERRORMACRO
}
void ComputeShader::setUniformFloat(const std::string& name, const float v){
    const int location = getUniformLocation(name);
    if (location == -1)
        return;
    glUniform1f(location, v);
    MYGLERRORMACRO
}

