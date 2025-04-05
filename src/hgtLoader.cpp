#include "hgtLoader.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

using namespace std;

HeightMap loadHgtFile(const string& filePath, short minValidHeight, short maxValidHeight, short noDataValue) {

    int width = 1201;
    int height = 1201;
    int totalSize = width * height;

    ifstream file(filePath, ios::binary); // probojemy otworzyc w trybie binarnym
    if (!file) {
        throw runtime_error("Failed to open HGT file: " + filePath);
    }

    vector<short> buffer(totalSize); // rezerwujemy pamiec na dane

    for (int i = 0; i < totalSize; i++) {

        uint8_t highByte, lowByte;
        if (!file.read(reinterpret_cast<char*>(&highByte), 1)) {
            throw runtime_error("Error reading highByte from: " + filePath);
        }
        if (!file.read(reinterpret_cast<char*>(&lowByte), 1)) {
            throw runtime_error("Error reading lowByte from: " + filePath);
        }
        // tutaj laczymy w wartosc
        short value = (static_cast<short>(highByte) << 8) | (lowByte);

        if (value < minValidHeight || value > maxValidHeight) { // sprawdzamy czy dane sa w przedziale
            value = noDataValue;
        }
        buffer[i] = value;
    }
    file.close();

    HeightMap hm; // koncowo z danych tworzymy Height mape
    hm.width = width;
    hm.height = height;
    hm.data = move(buffer); // "przekazujemy dane" zamiast kopiowac
    return hm;
}
