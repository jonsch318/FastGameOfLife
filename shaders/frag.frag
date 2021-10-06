#version 430 core

#define DATA_W 8192
#define DATA_H 8192

uniform vec2 WindowSize;

layout(std430 , binding=5) buffer Nex{
    int Next[DATA_W*DATA_H];
};

out vec4 fragColor;

void main(){
    //enablePrintf();
    int i = int((gl_FragCoord.x/WindowSize.x)*DATA_W);
    int j = int((gl_FragCoord.y/WindowSize.y)*DATA_H);

    //printf("x %d y %d a %d b %d \n", i, j, Next[i+j*DATA_W], Next[i+j*DATA_W] & 1); 

    if((Next[i+j*DATA_W] & 1) == 1){
        fragColor = vec4(0.0,0.0,0.0,1.0);
    } else {
        fragColor = vec4(1.0,1.0,1.0,1.0);
    }
}