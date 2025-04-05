#version 330 core

layout(location = 0) in int inHeight;

uniform int uSubW;
uniform int uStep;
uniform int uTileStartRow;
uniform int uTileStartCol;

uniform int uMegaWidth;
uniform int uMegaHeight;
uniform float uMinLat;
uniform float uMaxLat;
uniform float uMinLon;
uniform float uMaxLon;
uniform float uEarthRadius;
uniform mat4 uMVP;

out float vHeight;

void main() { // tu dosc analogicznie

    int vertexID = gl_VertexID;
    int localRow = vertexID / uSubW;
    int localCol = vertexID % uSubW;

    int bigRow = uTileStartRow + localRow * uStep;
    int bigCol = uTileStartCol + localCol * uStep;

    float rowNorm = 1.0f - (float(bigRow) / float(uMegaHeight - 1));
    float latDeg = mix(uMinLat, uMaxLat, rowNorm);

    float colNorm = float(bigCol) / float(uMegaWidth - 1);
    float lonDeg = mix(uMinLon, uMaxLon, colNorm);

    float altM = float(inHeight);

    float altKM = (altM + 500.0f) / 1000.0f;
    float radius = uEarthRadius + altKM;

    float latRad = radians(latDeg);
    float lonRad = radians(lonDeg);

    float x = radius * cos(latRad) * cos(lonRad);
    float y = radius * cos(latRad) * sin(lonRad);
    float z = radius * sin(latRad);

    vHeight = altM;

    gl_Position = uMVP * vec4(x, y, z, 1.0f);
}
