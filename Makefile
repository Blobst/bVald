CXX = clang++
CXXFLAGS = -std=c++23

# Directories
LIB_DIR = lib
INC_DIR = include
BUILD = build

shared:
	clang-cl /c include\libjsonval.cpp /Fo:libjsonval.obj
	llvm-lib libjsonval.obj /OUT:lib\libjsonval.lib

default: shared
	$(CXX) $(CXXFLAGS) main.cpp -L$(LIB_DIR) -I$(INC_DIR) -llibjsonval -o $(BUILD)\bvald.exe