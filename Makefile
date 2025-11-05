# Makefile for Blue Block Solver using OpenCV
CXX = g++
CXXFLAGS = -std=c++17 -g -Wall

# Detect Homebrew OpenCV path
OPENCV_PATH = /opt/homebrew/opt/opencv
# If you're on Intel Mac, it might be:
# OPENCV_PATH = /usr/local/opt/opencv

INCLUDES = -I$(OPENCV_PATH)/include/opencv4
LIBS = -L$(OPENCV_PATH)/lib -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lopencv_highgui -framework ApplicationServices -framework CoreGraphics

TARGET = solver
SRC = blue_block_solver.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)