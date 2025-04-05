#version 330 core

layout(location = 0) in int inHeight; 

uniform int uSubW; // zasadniczo to kolumny
uniform int uStep;         
uniform int uTileStartRow; 
uniform int uTileStartCol;

uniform int uMegaWidth;
uniform int uMegaHeight;
uniform mat4 uProjection;

uniform float uMinLat;
uniform float uMaxLat;
uniform float uMinLon;
uniform float uMaxLon;
uniform float uMidLat;

out float vHeight;

void main() {
    int vertexID = gl_VertexID; 
    int localRow = vertexID / uSubW; // np mamy 10 kolumn a gl_VertexID = 42 wtedy 42 / 10 = 4
    int localCol = vertexID % uSubW; // 42 % 10 = 2 ,zatem mamy row = 4 col = 2 latwiej bylo by narysowac 
                                     // ale dziala
    int bigRow = uTileStartRow + localRow * uStep;
    int bigCol = uTileStartCol + localCol * uStep;

    float rowNorm = float(bigRow) / float(uMegaHeight - 1);
    float latDeg = mix(uMaxLat, uMinLat, rowNorm);

    float colNorm = float(bigCol) / float(uMegaWidth -1);
    float lonDeg = mix(uMinLon, uMaxLon, colNorm);

    float cosMid = cos(radians(uMidLat));
    float x = (lonDeg - uMinLon) * cosMid;
    float y = (latDeg - uMinLat);

    vHeight = float(inHeight); 

    gl_Position = uProjection* vec4(x, y, 0.0f, 1.0f);
}
