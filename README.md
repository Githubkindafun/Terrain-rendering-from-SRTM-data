# Terrain-rendering-from-SRTM-data
Project developed as part of a computer graphics class.
The aim of the task is to display the actual terrain as accurately and quickly as possible based on the available DTED (digital terrain elevation data) elevation maps. The program should output the number of rendered triangles per second and the number of frames per second.
The terrain is to be read from SRTM data from the Space Shuttle Endeavour mission in 2000. This data includes ground surface elevations every few tens of metres.

Everything is implemented in C++ with OpenGL.

**Some pretty shots:**
![alpes](https://github.com/user-attachments/assets/0688fa8a-9578-41f2-af06-882c3c24daa5)
![alpes3D](https://github.com/user-attachments/assets/539e0bce-72b3-43be-930c-9927adf21525)
![poland](https://github.com/user-attachments/assets/f578d72e-7622-4591-a1c8-be52f0c4f6a8)


## Usage
There are 3 optional arguments:
* "-lat" x y
* "-lon" x y
* "-start" lon lat h (start location)

## Controls
### 2D
* WSAD: forward/backward/left/right
* +/-: zoom in/out
* 1/2/3/4: LoD level selection
* 0: auto LoD
* SPACE: 2D -> 3D

### 3D
* WSAD: forwad/backward/left/right
* +/-: speed up/down
* arrow keys UP/DOWN: up/down
* arrow keys LEFT/RIGHT: turning left/right
* 1/2/3/4: LoD level selection
* 0: auto LoD
* SPACE: 3D -> 2D
