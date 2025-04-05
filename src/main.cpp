#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"
#include "hgtLoader.h"

using namespace std;
using namespace glm;

int WIN_WIDTH = 1200;
int WIN_HEIGHT = 600;

float cameraX = 0.0f; // kamera 2D
float cameraY = 0.0f;
float cameraZoom = 1.0f;

vec3 camera3DPos = vec3(0.0f, 0.0f, 0.0f); // kamera 3D
vec3 camera3DFront = vec3(1.0f, 0.0f, 0.0f); 
vec3 camera3DUp = vec3(0.0f, 1.0f, 0.0f);
float yaw3D = 0.0f;
float pitch3D = 0.0f;
float moveSpeed3D = 300.0f;
float rotSpeed3D = 2.0f;

int currentLOD = 4; // LoD
bool autoLOD = false;
int numTiles = 1;

double lastTime = 0.0;
float lastFrame = 0.0f;
float deltaTime = 0.0f;

bool is3DView = false;

double gMinLon = 0.0;
double gMinLat = 0.0;
float  gMidLat = 0.0f;
double gMaxLat = 0.0;
double gMaxLon = 0.0;

const float EARTH_RADIUS = 6378.0f;

vec3 storedCamera3DPos;
vec3 storedCamera3DFront;
vec3 storedCamera3DUp;
float storedYaw3D;

const float crossSize = 10.0f;

unsigned int crossVAO;
unsigned int crossVBO;

struct RenderParams { // dla poszczegolengo kafelka
    int subW;
    int subH;
    int step;
    int tileStartRow;
    int tileStartCol;
};

struct TileBuffers { // dla kafelka
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int indexCount = 0;
    RenderParams params;
};

struct SphereMesh {
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    int indexCount = 0;
};

struct TileInfo {
    int lat;
    int lon;
    int tileY;
    int tileX;
    HeightMap hm;  
};

struct SharedEBO { // wspoldzielony EBO
    unsigned int eboID = 0;
    int indexCount = 0;
};

struct ComputationResults {
    vector<short> heights; // tutaj trzymane sa wysokosci
};


int clampCoord(int val, int maxVal);

ComputationResults computeTileVertices(const vector<short>& megaData, int megaWidth, int megaHeight, int tileStartRow,
    int tileStartCol, int tileSize, int step);

SharedEBO buildSharedEBOForLOD(int tileSize, int step);

TileBuffers buildTileBuffers(const vector<short>& megaData, int megaWidth, int megaHeight, int tileY, int tileX,
    int tileSize, int step, unsigned int sharedEBO, int sharedIndexCount);

SphereMesh buildBaseSphere(float baseRadiusKM, int vSlices = 36, int hSlices = 18);

bool parseHgtFilename(const string& fname, int& lat, int& lon);

int latToIndex(int lat, int maxLat);

int lonToIndex(int lon, int minLon);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

void updateFPSandPrint(int triCount);

vec3 convertGeoToCartesian(double lon, double lat, double heightKM);

void sync2Dto3D(double minLon, double minLat, float midLat);

void sync3Dto2D(double minLon, double minLat, float midLat, double maxLat, double maxLon);

void place2DCameraFromStart(double startLon, double startLat, double minLon, double minLat, float midLat);


int clampCoord(int val, int maxVal) {

    if (val < 0) {
        return 0;
    } else if (val >= maxVal) {
        return maxVal - 1;
    } else {
        return val;
    }
}

ComputationResults computeTileVertices(const vector<short>& megaData, int megaWidth, int megaHeight, int tileStartRow,
    int tileStartCol, int tileSize, int step) { // z megaDaty wgrywamy wysokosci
    
    int baseH = (tileSize - 1) / step + 1; // rozmiar kafelka
    int baseW = (tileSize - 1) / step + 1;

    int subH = baseH + 1;
    int subW = baseW + 1;

    ComputationResults cr;
    cr.heights.reserve(subH * subW);

    for(int r = 0; r < subH; r++) {

        int bigR = clampCoord(tileStartRow + r * step, megaHeight);

        for(int c = 0; c < subW; c++) {

            int bigC = clampCoord(tileStartCol + c * step, megaWidth);
            int bigIdx = bigR * megaWidth + bigC;
            short alt = megaData[bigIdx];
            cr.heights.push_back(alt);
        }
    }
    return cr;
}

SharedEBO buildSharedEBOForLOD(int tileSize, int step) { // tutaj tworzymy dla kazdego LoD
                                                         // jeden wspolny EBO skoro i tak kazdy ma ta sama
    int baseH = (tileSize - 1) / step + 1;               // strukture
    int baseW = (tileSize - 1) / step + 1;

    int subH = baseH + 1;
    int subW = baseW + 1;

    vector<int> indices;
    indices.reserve(6 * (subH - 1) * (subW - 1)); // rezerwujemy sobie pamiec

    for(int row = 0; row < (subH - 1); row++) {
        for(int col = 0; col < (subW - 1); col++) {
            int idxTL = row * subW + col; // gorny lewy
            int idxTR = idxTL + 1; // etc.
            int idxBL = (row + 1) * subW + col;
            int idxBR = idxBL + 1;

            indices.push_back(idxTL);
            indices.push_back(idxBL); // pierwszy trojkat
            indices.push_back(idxBR);

            indices.push_back(idxTL);
            indices.push_back(idxBR); // drugi trojkat
            indices.push_back(idxTR);
        }
    }

    SharedEBO se;
    se.indexCount = (int)indices.size();

    glGenBuffers(1, &se.eboID);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, se.eboID);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return se;
}

TileBuffers buildTileBuffers(const vector<short>& megaData, int megaWidth, int megaHeight, int tileY, int tileX,
    int tileSize, int step, unsigned int sharedEBO, int sharedIndexCount) {

    int baseH = (tileSize - 1) / step + 1;
    int baseW = (tileSize - 1) / step + 1;

    int subH = baseH + 1;
    int subW = baseW + 1;

    int tileStartRow = tileY * tileSize;
    int tileStartCol = tileX * tileSize;

    ComputationResults cr = computeTileVertices(megaData, megaWidth, megaHeight, tileStartRow, tileStartCol,
                                tileSize, step); // wyliczamy wysokosci

    TileBuffers tb;
    tb.indexCount = sharedIndexCount;

    tb.params.subW = subW;
    tb.params.subH = subH;
    tb.params.step = step;
    tb.params.tileStartRow = tileStartRow;
    tb.params.tileStartCol = tileStartCol;

    glGenVertexArrays(1, &tb.VAO);
    glGenBuffers(1, &tb.VBO);

    glBindVertexArray(tb.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, tb.VBO);
    glBufferData(GL_ARRAY_BUFFER, cr.heights.size() * sizeof(short), cr.heights.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO);
    glVertexAttribIPointer(0, 1, GL_SHORT, sizeof(short), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return tb;
}

SphereMesh buildBaseSphere(float baseRadiusKM, int vSlices, int hSlices) { // tutaj nic specjalnego analogiczne 
                                                                           // do poprzednich zadan zwykla kulka
    vector<float> positions;
    vector<int> indices;

    float horizontalStep = M_PI / (float)hSlices;
    float verticalStep = 2.0f * M_PI / (float)vSlices;

    for(int i = 0; i <= hSlices; i++) {

        float theta = i * horizontalStep;
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);

        for(int j = 0; j <= vSlices; j++) {
            float phi = j * verticalStep;
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);

            float x = baseRadiusKM * sinTheta * cosPhi;
            float y = baseRadiusKM * cosTheta;
            float z = baseRadiusKM * sinTheta * sinPhi;
            positions.push_back(x);
            positions.push_back(y);
            positions.push_back(z);
        }
    }

    for(int i = 0; i < hSlices; i++) {
        int currSlice = i * (vSlices + 1);
        int nextSlice = currSlice + (vSlices + 1);

        for(int j = 0; j < vSlices; j++, currSlice++, nextSlice++) {
            
            if(i != 0) {
                indices.push_back(currSlice);
                indices.push_back(nextSlice);
                indices.push_back(currSlice + 1);
            }
            if(i != (hSlices - 1)) {
                indices.push_back(currSlice +1);
                indices.push_back(nextSlice);
                indices.push_back(nextSlice +1);
            }
        }
    }

    SphereMesh sm;
    sm.indexCount = indices.size();

    glGenVertexArrays(1, &sm.VAO);
    glGenBuffers(1, &sm.VBO);
    glGenBuffers(1, &sm.EBO);

    glBindVertexArray(sm.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, sm.VBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sm.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return sm;
}


bool parseHgtFilename(const string& fname, int& lat, int& lon) {

    if (fname.size() < 9) {
        return false;
    }

    char latSign = fname[0];
    string latStr = fname.substr(1, 2);
    char lonSign = fname[3];
    string lonStr = fname.substr(4, 3);

    if ((latSign != 'N' && latSign != 'S') || (lonSign != 'E' && lonSign != 'W')) {
        return false;
    }

    lat = stoi(latStr);
    lon = stoi(lonStr);

    if (latSign == 'S') {
        lat = -lat;
    }
    if (lonSign == 'W') {
        lon = -lon;
    }
    return true;
}

int latToIndex(int lat, int maxLat) {
    return (maxLat - lat);
}

int lonToIndex(int lon, int minLon) {
    return (lon - minLon);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {

    WIN_WIDTH = width;
    WIN_HEIGHT = height;

    glViewport(0, 0, width, height);

    if(!is3DView) {
        float cosMid = cos(radians(gMidLat));
        float dataW = (gMaxLon - gMinLon) * cosMid;
        float dataH = (gMaxLat - gMinLat);

        if(dataW < 1.0f) dataW = 1.0f;
        if(dataH < 1.0f) dataH = 1.0f;

        float zoomX = (float)WIN_WIDTH / dataW;
        float zoomY = (float)WIN_HEIGHT / dataH;

        if (zoomX < zoomY) {
            cameraZoom = zoomX;
        } else {
            cameraZoom = zoomY;
        }

        sync3Dto2D(gMinLon, gMinLat, gMidLat, gMaxLat, gMaxLon);
    }
}



double lastTimeMain = 0.0;
int frameCountMain = 0;
double lodCooldown = 0.0;
const double COOLDOWN_TIME = 5.0;

void updateFPSandPrint(int triCount) {

    double curr = glfwGetTime();
    frameCountMain++;

    if((curr - lastTimeMain) >= 1.0) {
        double fps = (double)frameCountMain / (curr - lastTimeMain);
        frameCountMain = 0;
        lastTimeMain = curr;

        
        cout << "FPS: " << fps << ", MLN Triangles = " << (triCount / 1000000.0f) << ", LOD = " << currentLOD << endl;

        if((curr - lodCooldown) >= COOLDOWN_TIME && autoLOD) {

            if(fps < 10.0f && currentLOD < 4) {
                currentLOD++;
                lodCooldown = curr;
                cout << "FPS below 10 => LOD = " << currentLOD << endl;
            } 
            else if(fps >= 10.0f && currentLOD > 1) {
                currentLOD--;
                lodCooldown = curr;
                cout << "FPS above 10 => LOD = " << currentLOD << endl;
            }
        }
    }
}

vec3 convertGeoToCartesian(double lon, double lat, double heightKM) {

    double lonRad = radians(lon - 0.35f); // lekki offset mi wychodzil
    double latRad = radians(lat - 0.83f); // po delikatnych eksperymentach to w miare dobrze poprawia 
    double radius = EARTH_RADIUS + heightKM; // oblicznia

    float x = (float)(radius * cos(latRad) * cos(lonRad));
    float y = (float)(radius * cos(latRad) * sin(lonRad));
    float z = (float)(radius * sin(latRad));
    
    return vec3(x, y, z);
}

void sync2Dto3D(double minLon, double minLat, float midLat) { // tutaj po to aby przechowywac stan i go dobrze
                                                              // odtwarzac z powrotem
    camera3DPos = storedCamera3DPos;
    camera3DFront = storedCamera3DFront;
    camera3DUp = storedCamera3DUp;
    yaw3D = storedYaw3D;
}

void sync3Dto2D(double minLon, double minLat, float midLat, double maxLat, double maxLon) {

    float radius = length(camera3DPos);
    float altKM = radius - EARTH_RADIUS; 

    float latRad = asinf(camera3DPos.z / radius); // arc bo szukamy kata
    float lonRad = atan2(camera3DPos.y, camera3DPos.x);

    double latDeg = degrees(latRad);
    double lonDeg = degrees(lonRad);

    latDeg = std::clamp(latDeg, (double)minLat, (double)maxLat);
    lonDeg = std::clamp(lonDeg, (double)minLon, (double)maxLon);

    float x2D = (float)((lonDeg - minLon) * cos(radians(midLat)));
    float y2D = (float)(latDeg - minLat);

    cameraZoom = ((1000.0f / (altKM + 1.0f)) * 45.0f);

    float viewW = (float)WIN_WIDTH  / cameraZoom;
    float viewH = (float)WIN_HEIGHT / cameraZoom;

    cameraX = x2D - 0.5f * viewW;
    cameraY = y2D - 0.5f * viewH;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    if(action == GLFW_PRESS || action == GLFW_REPEAT) {
        float speed2D = 200.0f;
        float zoomMult = 1.1f;

        switch(key) {
            case GLFW_KEY_W:
                if(!is3DView) {
                    cameraY += (speed2D / cameraZoom) * deltaTime;
                } else {
                    camera3DPos += camera3DFront * (moveSpeed3D * deltaTime);
                }
                break;
            case GLFW_KEY_S:
                if(!is3DView) {
                    cameraY -= (speed2D / cameraZoom) * deltaTime;
                } else {
                    camera3DPos -= camera3DFront * (moveSpeed3D * deltaTime);
                }
                break;
            case GLFW_KEY_A:
                if(!is3DView) {
                    cameraX -= (speed2D / cameraZoom) * deltaTime;
                } else {
                    vec3 left = normalize(cross(camera3DUp, camera3DFront));
                    camera3DPos += left * (moveSpeed3D * deltaTime);
                }
                break;
            case GLFW_KEY_D:
                if(!is3DView) {
                    cameraX += (speed2D / cameraZoom) * deltaTime;
                } else {
                    vec3 right = normalize(cross(camera3DFront, camera3DUp));
                    camera3DPos += right * (moveSpeed3D * deltaTime);
                }
                break;
            case GLFW_KEY_EQUAL:
            case GLFW_KEY_KP_ADD:
                if(!is3DView) {
                    float oldW = (float)WIN_WIDTH / cameraZoom;
                    float oldH = (float)WIN_HEIGHT / cameraZoom;
                    cameraZoom *= zoomMult;
                    float newW = (float)WIN_WIDTH / cameraZoom;
                    float newH = (float)WIN_HEIGHT / cameraZoom;
                    cameraX += 0.5f * (oldW - newW);
                    cameraY += 0.5f * (oldH - newH);
                } else {
                    moveSpeed3D += moveSpeed3D * 0.1f;
                }
                break;
            case GLFW_KEY_MINUS:
            case GLFW_KEY_KP_SUBTRACT:
                if(!is3DView) {
                    float oldW = (float)WIN_WIDTH / cameraZoom;
                    float oldH = (float)WIN_HEIGHT / cameraZoom;
                    cameraZoom /= zoomMult;
                    float newW = (float)WIN_WIDTH / cameraZoom;
                    float newH = (float)WIN_HEIGHT / cameraZoom;
                    cameraX += 0.5f * (oldW - newW);
                    cameraY += 0.5f * (oldH - newH);
                } else {
                    moveSpeed3D -= moveSpeed3D * 0.1f;
                    if(moveSpeed3D < 10.0f) {
                        moveSpeed3D = 10.0f;
                    }
                }
                break;
            case GLFW_KEY_UP:
                if(is3DView) {
                    float altitudeChange = moveSpeed3D * deltaTime;
                    float currentRadius = length(camera3DPos);
                    float newRadius = currentRadius + altitudeChange;
                    camera3DPos = normalize(camera3DPos) * newRadius;
                }
                break;
            case GLFW_KEY_DOWN:
                if(is3DView) {
                    float altitudeChange = moveSpeed3D * deltaTime;
                    float currentRadius = length(camera3DPos);
                    float newRadius = currentRadius - altitudeChange;
                    camera3DPos = normalize(camera3DPos) * newRadius;
                }
                break;
            case GLFW_KEY_LEFT:
                if(is3DView) {
                    yaw3D -= rotSpeed3D * (moveSpeed3D / 2) * deltaTime;
                }
                break;
            case GLFW_KEY_RIGHT:
                if(is3DView) {
                    yaw3D += rotSpeed3D * (moveSpeed3D / 2) * deltaTime;
                }
                break;
            case GLFW_KEY_SPACE:
                if(action == GLFW_PRESS) {
                    is3DView = !is3DView;
                    if(is3DView) {
                        sync2Dto3D(gMinLon, gMinLat, gMidLat);
                        cout << "Switched to 3D.\n";
                    } else {
                        storedCamera3DPos = camera3DPos;
                        storedCamera3DFront = camera3DFront;
                        storedCamera3DUp = camera3DUp;
                        storedYaw3D = yaw3D;

                        sync3Dto2D(gMinLon, gMinLat, gMidLat, gMaxLat, gMaxLon);
                        cout << "Switched to 2D.\n";
                    }
                }
                break;
            case GLFW_KEY_0:
                if(action == GLFW_PRESS) {
                    autoLOD = true;
                    cout << "Automatic LOD ON\n";
                }
                break;
            case GLFW_KEY_1:
            case GLFW_KEY_2:
            case GLFW_KEY_3:
            case GLFW_KEY_4:
                if(action == GLFW_PRESS) {

                    autoLOD = false;
                    int typedLOD = key - GLFW_KEY_0; // GLFW_KEY_0 to 48
                    if(typedLOD < 1) {
                        typedLOD = 1;
                    } else if(typedLOD > 4) {
                        typedLOD = 4;
                    }
                    currentLOD = typedLOD;
                    cout << "Manual LOD => " << currentLOD << endl;
                }
                break;
            case GLFW_KEY_ESCAPE:
                if(action == GLFW_PRESS) {
                    glfwSetWindowShouldClose(window, true);
                }
                break;
        }
    }
}

void place2DCameraFromStart(double startLon, double startLat, double minLon, double minLat, float midLat) {
    
    float cosMid = cos(radians(midLat));
    float x2D = (float)(startLon - minLon) * cosMid - 0.35f;
    float y2D = (float)(startLat - minLat) - 0.83f;

    float viewW = (float)WIN_WIDTH / cameraZoom;
    float viewH = (float)WIN_HEIGHT / cameraZoom;

    cameraX = x2D - 0.5f * viewW;
    cameraY = y2D - 0.5f * viewH;
}


int main(int argc, char** argv) {

    if(argc < 2) {
        cout << "Usage: " << argv[0] << " <folder> [ -lon min max ] [ -lat min max ] [ -start lon lat h ]\n";
        return -1;
    }
    string folderPath = argv[1];

    int userLonMin = -9999; // zmienne dla opcjonalnych arg
    int userLonMax = 9999;
    int userLatMin = -9999;
    int userLatMax = 9999;
    bool haveLonRange = false;
    int haveLatRange = false;

    double startLon = 0.0;
    double startLat = 0.0;
    double startH = 0.0;
    bool hasStart = false;

    
    for(int i = 2; i < argc; i++) { // sciagamy arg z consli
        string arg = argv[i];
        if(arg == "-lon" && i + 2 < argc) {
            userLonMin = stoi(argv[++i]);
            userLonMax = stoi(argv[++i]);
            haveLonRange = true;
        }
        else if(arg == "-lat" && i + 2 < argc) {
            userLatMin = stoi(argv[++i]);
            userLatMax = stoi(argv[++i]);
            haveLatRange = true;
        }
        else if(arg == "-start" && i + 3 < argc) {
            startLon = stod(argv[++i]);
            startLat = stod(argv[++i]);
            startH = stod(argv[++i]);
            hasStart = true;
            cout << "Start => Lon : " << startLon << " Lat : " << startLat << " H : " << startH << endl;
        } else {
            cout << "Unrecognized arg: " << arg << endl;
        }
    }

    vector<string> hgtFiles; // wyciagamy pliki z katalogu
    for(const auto &entry: filesystem::directory_iterator(folderPath)) {
        if(entry.path().extension() == ".hgt") {
            hgtFiles.push_back(entry.path().string());
        }
    }
    if(hgtFiles.empty()) {
        cout << "No .hgt files found.\n";
        return -1;
    }

    int minLat = 9999;
    int maxLat = -9999;
    int minLon = 9999;
    int maxLon = -9999;

    for(const auto &path: hgtFiles) { // tutaj ustalamy granice 
        string fname = filesystem::path(path).filename().string();
        int lat = 0;
        int lon = 0;
        if(!parseHgtFilename(fname, lat, lon)) {
            cout << "Parse fail " << fname << endl;
            continue;
        }
        if(haveLatRange) {
            if(lat < userLatMin || lat > userLatMax) {
                continue;
            }
        }
        if(haveLonRange) {
            if(lon < userLonMin || lon > userLonMax) {
                continue;
            }
        }
        if(lat < minLat) {
            minLat = lat;
        }
        if(lat > maxLat) {
            maxLat = lat;
        }
        if(lon < minLon) {
            minLon = lon;
        }
        if(lon > maxLon) {
            maxLon = lon;
        }
    }
    if(minLat == maxLat) {
        maxLat = minLat + 1;
    }
    if(minLon == maxLon) {
        maxLon = minLon + 1;
    }

    vector<TileInfo> tiles; // tutaj wczytujemy dane z plikow
    for(const auto &path: hgtFiles) {
        string fname = filesystem::path(path).filename().string();
        int lat = 0;
        int lon = 0;
        if(!parseHgtFilename(fname, lat, lon)) {
            continue;
        }
        if(haveLatRange) {
            if(lat < userLatMin || lat > userLatMax) {
                continue;
            }
        }
        if(haveLonRange) {
            if(lon < userLonMin || lon > userLonMax) {
                continue;
            }
        }

        HeightMap hm; // tu trzymamy dane
        try {
            hm = loadHgtFile(path, -500, 9000, -9999);
        } catch(...) {
            cout << "Error loading " << fname << endl;
            continue;
        }
        int tileY = latToIndex(lat, maxLat);
        int tileX = lonToIndex(lon, minLon);

        TileInfo tinfo {lat, lon, tileY, tileX, move(hm)}; // przelewamy dane do tile'ow
        tiles.push_back(tinfo);
    }
    if(tiles.empty()) {
        cout << "No valid tiles after filtering.\n";
        return -1;
    }
    numTiles = tiles.size();

    const int tileSize = 1201;
    int latCount = (maxLat - minLat) + 1;
    int lonCount = (maxLon - minLon) + 1;
    int megaWidth = lonCount * tileSize;
    int megaHeight = latCount * tileSize;

    vector<short> megaData(megaWidth * megaHeight, -9999);
    for(auto &t: tiles) { // tutaj wgrywamy dane z tile'ow do mega data

        int offsetX = t.tileX * tileSize;
        int offsetY = t.tileY * tileSize;
        for(int r = 0; r < tileSize; r++) {
            for(int c = 0; c < tileSize; c++) {

                short val = t.hm.data[r * tileSize + c];
                int bigR = offsetY + r;
                int bigC = offsetX + c;
                megaData[bigR * megaWidth + bigC] = val;
            }
        }
    }

    short globalMin = 32767; // tutaj ustalamy globalna amplitude wysokosci
    short globalMax = -32768;
    int validCount = 0;

    for(auto v: megaData) {
        if(v == -9999) {
            continue;
        }
        validCount++;
        if(v < globalMin) {
            globalMin = v;
        }
        if(v > globalMax) {
            globalMax = v;
        }
    }
    if(globalMin == 32767) {
        globalMin = -500;
        globalMax = 9000;
    }

    cout << "Valid alt points: " << validCount << " / " << megaData.size() << "\n"
        << "Global height range: [" << globalMin << "..." << globalMax << "]\n";

    if(!glfwInit()) { // standard
        cerr << "GLFW init fail.\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Z6", NULL, NULL);
    if(!window) {
        cerr << "Failed to create window.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if(glewInit() != GLEW_OK) {
        cerr << "GLEW init fail.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    float crossVertices[] = { // tutaj mamy "+" co jest na srodku ekranu
        -crossSize, 0.0f,
        crossSize, 0.0f,
        0.0f, -crossSize,
        0.0f, crossSize };

    glGenVertexArrays(1, &crossVAO);
    glGenBuffers(1, &crossVBO);

    glBindVertexArray(crossVAO);

    glBindBuffer(GL_ARRAY_BUFFER, crossVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crossVertices), crossVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    float midLat = 0.5f * ((float)minLat + (float)maxLat);
    float cosMid = cos(radians(midLat));
    float dataW = (maxLon - minLon) * cosMid;
    float dataH = (maxLat - minLat);
    if(dataW < 1.0f) {
        dataW = 1.0f;
    }
    if(dataH < 1.0f) {
        dataH = 1.0f;
    }
    float zoomX = (float)WIN_WIDTH / dataW;
    float zoomY = (float)WIN_HEIGHT / dataH;
    if (zoomX < zoomY) {
        cameraZoom = zoomX;
    } else {
        cameraZoom = zoomY;
    }

    float viewW = (float)WIN_WIDTH / cameraZoom;
    float viewH = (float)WIN_HEIGHT / cameraZoom;
    float midX = dataW * 0.5f;
    float midY = dataH * 0.5f;

    cameraX = midX - 0.5f * viewW;
    cameraY = midY - 0.5f * viewH;

    gMinLon = (double)minLon;  
    gMinLat = (double)minLat;  
    gMidLat = midLat;
    gMaxLat = (double)maxLat;
    gMaxLon = (double)maxLon;

     if(hasStart) {

        camera3DPos = convertGeoToCartesian(startLon, startLat, startH);
        camera3DFront = normalize(-camera3DPos);
        
        yaw3D = degrees(atan2(camera3DFront.z, camera3DFront.x)); // rotacja kamey na x-z

        storedCamera3DPos = camera3DPos;
        storedCamera3DFront = camera3DFront;
        storedCamera3DUp = camera3DUp;
        storedYaw3D = yaw3D;

        place2DCameraFromStart(startLon, startLat, (double)minLon, (double)minLat, midLat);

    } else {

        double centerLat = 0.5 * (minLat + maxLat);
        double centerLon = 0.5 * (minLon + maxLon);

        vec3 centerCart = convertGeoToCartesian(centerLon, centerLat, 0.0);

        camera3DPos = convertGeoToCartesian(centerLon, centerLat, 50.0);

        camera3DFront = normalize(centerCart - camera3DPos);

        yaw3D = degrees(atan2(camera3DFront.z, camera3DFront.x)); 

        storedCamera3DPos = camera3DPos;
        storedCamera3DFront = camera3DFront;
        storedCamera3DUp = camera3DUp;
        storedYaw3D = yaw3D;

        sync3Dto2D(gMinLon, gMinLat, gMidLat, gMaxLat, gMaxLon);
    }

    vector<SharedEBO> sharedLodEBOs(5);
    for(int lod = 1; lod <= 4; lod++) {
        int step = 1;
        if(lod == 1) {
            step = 1;
        } else if(lod == 2) {
            step = 2;
        } else if(lod == 3) {
            step = 4;
        } else { 
            step = 8;
        }

        sharedLodEBOs[lod]= buildSharedEBOForLOD(tileSize, step);
    }

    vector<vector<TileBuffers>> lodChunks(5);
    for(int lod = 1; lod <= 4; lod++) {
        int step = 1;
        if(lod == 1) {
            step = 1;
        } else if(lod == 2) {
            step = 2;
        } else if(lod == 3) {
            step = 4;
        } else {
            step = 8;
        }
        SharedEBO &ebo = sharedLodEBOs[lod];

        for(auto &t: tiles) {
            TileBuffers tb = buildTileBuffers(megaData, megaWidth, megaHeight, t.tileY, t.tileX, tileSize,
                step, ebo.eboID, ebo.indexCount);

            lodChunks[lod].push_back(tb);
        }
        cout << "LOD " << lod << " => built " << lodChunks[lod].size() << " tiles\n";
    }

    SphereMesh baseSphere = buildBaseSphere(EARTH_RADIUS, 40, 80);

    Shader mapShader("src/shaders/mapVertex.glsl", "src/shaders/mapFragment.glsl");
    Shader heightShader("src/shaders/hgtSphereVertex.glsl", "src/shaders/hgtSphereFragment.glsl");
    Shader baseShader("src/shaders/baseSphereVertex.glsl", "src/shaders/baseSphereFragment.glsl");
    Shader crossShader("src/shaders/crossVertex.glsl", "src/shaders/crossFragment.glsl");


    lastTime = glfwGetTime();
    glEnable(GL_DEPTH_TEST);

    cout << "minLat = " << minLat  << ", maxLat = " << maxLat  << ", minLon = " << minLon  << ", maxLon = " << maxLon << endl;

    while(!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

        if(!is3DView) {
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            int drawLOD = currentLOD;
            if(autoLOD) {
                drawLOD = currentLOD; 
            }

            float vw = (float)WIN_WIDTH / cameraZoom;
            float vh = (float)WIN_HEIGHT / cameraZoom;
            mat4 proj = ortho(cameraX, cameraX + vw, cameraY, cameraY + vh, -1.0f, 1.0f);

            mapShader.use();
            mapShader.setMat4("uProjection", proj);
            mapShader.setFloat("uMinHeight", (float)globalMin);
            mapShader.setFloat("uMaxHeight", (float)globalMax);
            mapShader.setInt("uMegaWidth", megaWidth);
            mapShader.setInt("uMegaHeight", megaHeight);
            mapShader.setFloat("uMinLat", (float)minLat);
            mapShader.setFloat("uMaxLat", (float)maxLat);
            mapShader.setFloat("uMinLon", (float)minLon);
            mapShader.setFloat("uMaxLon", (float)maxLon);
            mapShader.setFloat("uMidLat", midLat);

            int totalIndices = 0;
            for(auto &tb : lodChunks[drawLOD]) {
                mapShader.setInt("uSubW", tb.params.subW);
                mapShader.setInt("uStep", tb.params.step);
                mapShader.setInt("uTileStartRow", tb.params.tileStartRow);
                mapShader.setInt("uTileStartCol", tb.params.tileStartCol);

                glBindVertexArray(tb.VAO);
                glDrawElements(GL_TRIANGLES, tb.indexCount, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                totalIndices += tb.indexCount;
            }
            updateFPSandPrint(totalIndices / 3);

            crossShader.use();
            mat4 crossProj = ortho(0.0f, (float)WIN_WIDTH, 0.0f, (float)WIN_HEIGHT, -1.0f, 1.0f);
            crossShader.setMat4("uProjection", crossProj);

            mat4 crossModel = translate(mat4(1.0f), vec3(WIN_WIDTH / 2.0f, WIN_HEIGHT / 2.0f, 0.0f));
            crossShader.setMat4("uModel", crossModel);

            glBindVertexArray(crossVAO);
            glDrawArrays(GL_LINES, 0, 4);
            glBindVertexArray(0);

        } else {

            glEnable(GL_DEPTH_TEST);
            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            vec3 up = normalize(camera3DPos); 
            camera3DUp = up;

            vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);
            vec3 east = normalize(cross(worldUp, up)); // prawo dla kamery
            if(length(east) < 0.001f) { 
                east = vec3(1.0f, 0.0f, 0.0f);
            }
            vec3 north = normalize(cross(up, east)); // "do przodu" dla kamery

            float yawRad = radians(yaw3D);
            vec3 front = cos(yawRad) * north + sin(yawRad) * east; // obracanie na plaszczyznie
                                                        // zasadniczo kierunek dla kamery w ukladzie
            camera3DFront = normalize(front);           // wsp north east (lokalny uklad kamery)
            // https://stackoverflow.com/questions/43101655/aligning-an-object-to-the-surface-of-a-sphere-while-maintaining-forward-directio

            mat4 view = lookAt(camera3DPos, camera3DPos + camera3DFront, camera3DUp);
            float aspect = (float)WIN_WIDTH / (float)WIN_HEIGHT;
            mat4 proj = perspective(radians(45.0f), aspect, 1.0f, 20000.0f);

            int selectedLOD = currentLOD;
            if(autoLOD) {
                selectedLOD = currentLOD; 
            }

            mat4 model = mat4(1.0f);
            mat4 mvp = proj * view * model;

            heightShader.use();
            heightShader.setMat4("uMVP", mvp);
            heightShader.setInt("uMegaWidth", megaWidth);
            heightShader.setInt("uMegaHeight", megaHeight);
            heightShader.setFloat("uMinLat", (float)minLat);
            heightShader.setFloat("uMaxLat", (float)maxLat);
            heightShader.setFloat("uMinLon", (float)minLon);
            heightShader.setFloat("uMaxLon", (float)maxLon);
            heightShader.setFloat("uEarthRadius", EARTH_RADIUS);
            heightShader.setFloat("uMinHeight", (float)globalMin);
            heightShader.setFloat("uMaxHeight", (float)globalMax);

            int totalIndices = 0;
            for(auto &tb : lodChunks[selectedLOD]) {
                heightShader.setInt("uSubW", tb.params.subW);
                heightShader.setInt("uStep", tb.params.step);
                heightShader.setInt("uTileStartRow", tb.params.tileStartRow);
                heightShader.setInt("uTileStartCol", tb.params.tileStartCol);

                glBindVertexArray(tb.VAO);
                glDrawElements(GL_TRIANGLES, tb.indexCount, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                totalIndices += tb.indexCount;
            }

            baseShader.use();
            mat4 mvpBase = proj * view;
            baseShader.setMat4("uMVP", mvpBase);
            baseShader.setMat4("uModel", mat4(1.0f));
            baseShader.setVec3("uColor", vec3(40.0f / 256.0f, 70.0f / 256.0f, 89.0f / 256.0f));

            glBindVertexArray(baseSphere.VAO);
            glDrawElements(GL_TRIANGLES, baseSphere.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            
            crossShader.use();
            mat4 crossProj = ortho(0.0f, (float)WIN_WIDTH, 0.0f, (float)WIN_HEIGHT, -1.0f, 1.0f);
            crossShader.setMat4("uProjection", crossProj);

            mat4 crossModel = translate(mat4(1.0f), vec3(WIN_WIDTH / 2.0f, WIN_HEIGHT / 2.0f, 0.0f));
            crossShader.setMat4("uModel", crossModel);

            glBindVertexArray(crossVAO);
            glDrawArrays(GL_LINES, 0, 4);
            glBindVertexArray(0);

            updateFPSandPrint((totalIndices + baseSphere.indexCount) / 3);
        }

        glfwSwapBuffers(window);
    }

    for(int lod = 1; lod <= 4; lod++) { // sprzatamy
        for(auto &tb : lodChunks[lod]) {
            glDeleteVertexArrays(1, &tb.VAO);
            glDeleteBuffers(1, &tb.VBO);
        }
        glDeleteBuffers(1, &sharedLodEBOs[lod].eboID);
    }

    glDeleteVertexArrays(1, &baseSphere.VAO);
    glDeleteBuffers(1, &baseSphere.VBO);
    glDeleteBuffers(1, &baseSphere.EBO);

    glDeleteVertexArrays(1, &crossVAO);
    glDeleteBuffers(1, &crossVBO);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}