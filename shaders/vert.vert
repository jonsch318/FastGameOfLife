#version 430 core

layout (location = 0) in vec4 vertexPosition;

out vec4 out_color;

layout(std430 , binding=5) buffer Nex{
    int Next[1000][1000];
    //uint Next[200][200];
};

const int factor_w = 1000/2;
const int factor_h = 1000/2;

void main(){


    gl_Position.x = vertexPosition.x;
    gl_Position.y = vertexPosition.y;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;

    int i = int(vertexPosition.z);
    int j = int(vertexPosition.w);

    if(Next[i][j] == 1){
        out_color = vec4(0.0,0.0,0.0,1.0);
    }
    if(Next[i][j] == 0){
        out_color = vec4(1.0,1.0,1.0,1.0);
    }
}