#version 430 core

#define DATA_W 512
#define DATA_H 512

uniform vec2 WindowSize;
uniform vec4 MapOffset;

layout(std430 , binding=5) buffer Nex{
    int Next[DATA_W*DATA_H];
};

out vec4 fragColor;

//n=temp; start1 = ZoomPoint.x, stop1 = ZoomSize.x, start2 = 0, stop2 DATA_W
float map(float n,float start1, float stop1, float start2, float stop2) {
  return ((n-start1)/(stop1-start1))*(stop2-start2)+start2;
}

void main(){

    int i = int(map(gl_FragCoord.x, 0, WindowSize.x, MapOffset.x, MapOffset.y));
    int j = int(map(gl_FragCoord.y, 0, WindowSize.y, MapOffset.z, MapOffset.w));

    if(i>=0 && j>=0 && i<DATA_W && j<DATA_H){
        fragColor = (Next[i+j*DATA_W] & 1) * vec4(1.0,1.0,1.0,1.0);
    }else {
        fragColor = vec4(0.0,0.0,0.0,1.0);
    }

}