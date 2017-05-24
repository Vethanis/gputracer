#include "myglheaders.h"
#include "timer.h"
#include "stdio.h"

Timer::Timer(){
    glGenQueries(1, &id);
}
Timer::~Timer(){
    glDeleteQueries(1, &id);
}
void Timer::begin(){
    glBeginQuery(GL_TIME_ELAPSED, id);
}
int Timer::end(){
    glEndQuery(GL_TIME_ELAPSED);
    int ns;
    glGetQueryObjectiv(id, GL_QUERY_RESULT, &ns);
    return ns / 1000000;
}
void Timer::endPrint(){
    printf("Timer ms: %i\n", this->end());
}
