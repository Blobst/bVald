# Compiler
CXX = clang++
CXXFLAGS = -std=c++23 -I$(INC_DIR)

# Directories
LIB_DIR = lib
INC_DIR = include
BUILD = build

# OS Detection
ifeq ($(OS),Windows_NT)
	OS_NAME := Windows
else
	OS_NAME := $(shell uname -s)
endif

# Executable names
ifeq ($(OS_NAME),Windows)
	EXE = $(BUILD)/bvald.exe
	LIB_FLAG = -llibjsonval
else
	EXE = $(BUILD)/bvald
	LIB_FLAG = -ljsonval
endif

# Create build and compile program depending on OS
default: $(EXE)

$(EXE): main.cpp | $(BUILD)
ifeq ($(OS_NAME),Windows)
	$(CXX) $(CXXFLAGS) main.cpp -L$(LIB_DIR) $(LIB_FLAG) -o $(EXE)
else ifeq ($(OS_NAME),Darwin)
	$(CXX) $(CXXFLAGS) main.cpp -L$(LIB_DIR) $(LIB_FLAG) -o $(EXE)
else ifeq ($(OS_NAME),Linux)
	$(CXX) $(CXXFLAGS) main.cpp -L$(LIB_DIR) $(LIB_FLAG) -o $(EXE)
endif

# Shared library build (Windows)
shared:
	clang-cl /c $(INC_DIR)\libjsonval.cpp /Fo:libjsonval.obj
	llvm-lib libjsonval.obj /OUT:$(LIB_DIR)\libjsonval.lib

# Create build folder automatically
$(BUILD):
	mkdir $(BUILD)

# ------------------ CLEAN ------------------
clean:
ifeq ($(OS_NAME),Windows)
	@if exist $(BUILD)\bvald.exe del /Q $(BUILD)\bvald.exe
	@if exist $(BUILD)\main.o del /Q $(BUILD)\main.o
	@if exist libjsonval.obj del /Q libjsonval.obj
	@if exist $(LIB_DIR)\libjsonval.lib del /Q $(LIB_DIR)\libjsonval.lib
else
	rm -rf $(BUILD)/bvald $(BUILD)/main.o libjsonval.obj $(LIB_DIR)/libjsonval.lib
endif
