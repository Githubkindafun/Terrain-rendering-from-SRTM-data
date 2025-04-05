# Terrain-rendering-from-SRTM-data
Project developed as part of a computer graphics class.
The aim of the task is to display the actual terrain as accurately and quickly as possible based on the available DTED (digital terrain elevation data) elevation maps. The program should output the number of rendered triangles per second and the number of frames per second.
The terrain is to be read from SRTM data from the Space Shuttle Endeavour mission in 2000. This data includes ground surface elevations every few tens of metres.

Everything is implemented in C++ with OpenGL.

**Some pretty shots:**


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