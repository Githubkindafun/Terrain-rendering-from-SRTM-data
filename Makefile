CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall


LIBS = -lGLEW -lglfw -lGL

SOURCES = src/main.cpp \
          src/hgtLoader.cpp

OBJECTS = $(SOURCES:.cpp=.o)
TARGET  = project

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
