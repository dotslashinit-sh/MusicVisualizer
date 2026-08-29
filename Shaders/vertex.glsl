#version 330 core

// layout(location = 0) in vec2 pos;
// layout(location = 1) in float height;

// void main() {
//    gl_Position = vec4(pos, 0.0, 1.0);
// }

uniform float barWidth;
uniform float barGap;
uniform float heightMax;

layout(location = 0) in float height;

// Vertex format:
// 0 (Bottom-left) -1.0, -1.0
// 1 (Bottom-right) 1.0, -1.0
// 2 (Top-left)    -1.0, 1.0
// 3 (Top-right)    1.0, 1.0

// Vertex ID mapping
// 0 - 0
// 1 - 1
// 2 - 2
// 3 - 1
// 4 - 2
// 5 - 3

void main() {
    float posX = -1.0 + (barWidth + barGap) * gl_InstanceID;
    if(gl_VertexID == 1 || gl_VertexID == 3 || gl_VertexID == 5) {
        posX += barWidth;
    }
    float posY = -1.0;
    if(gl_VertexID == 2 || gl_VertexID == 4 || gl_VertexID == 5) {
        posY += height/heightMax;
    }

    gl_Position = vec4(posX, posY, 0.0, 1.0);
}