#version 430 core

#define DATA_W 1000
#define DATA_H 1000

uniform vec2 WindowSize;

layout(std430 , binding=5) buffer Nex{
    int Next[DATA_W*DATA_H];
};

out vec4 fragColor;

void main(){
    int i = int(gl_FragCoord.x*WindowSize.x*DATA_W);
    int j = int(gl_FragCoord.y*WindowSize.y*DATA_H);

    fragColor = vec4(0.0,0.0,0.0,1.0) * (float(Next[i+j*DATA_W] == 1)) + vec4(1.0,1.0,1.0,1.0) * (float(Next[i+j*DATA_W] == 0));
}