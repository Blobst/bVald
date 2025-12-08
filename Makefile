# =============================================
# Compiler and directories
# =============================================
CXX = clang++
CXXFLAGS = -std=c++23 -I$(INC_DIR)

LIB_DIR = lib
INC_DIR = include
BUILD = build

SRC = main.cpp

# =============================================
# OS Detection
# =============================================
ifeq ($(OS),Windows_NT)
	OS_NAME := Windows
else
	OS_NAME := $(shell uname -s)
endif

# =============================================
# Platform specific output names
# =============================================
ifeq ($(OS_NAME),Windows)
	EXE = $(BUILD)/bvald.exe
	LIB_FLAG = -llibjsonval
	SHARED_LIB = $(LIB_DIR)/libjsonval.dll
	MKDIR = if not exist $(BUILD) mkdir $(BUILD)
else ifeq ($(OS_NAME),Darwin)
	EXE = $(BUILD)/bvald
	LIB_FLAG = -ljsonval
	SHARED_LIB = $(LIB_DIR)/libjsonval.dylib
	MKDIR = mkdir -p $(BUILD)
else
	EXE = $(BUILD)/bvald
	LIB_FLAG = -ljsonval
	SHARED_LIB = $(LIB_DIR)/libjsonval.so
	MKDIR = mkdir -p $(BUILD)
endif

# =============================================
# Default build
# =============================================
default: $(EXE)

$(EXE): $(SRC) | prepare
	@echo "Building for $(OS_NAME)..."
	$(CXX) $(CXXFLAGS) $(SRC) -L$(LIB_DIR) $(LIB_FLAG) -o $(EXE)
	@echo "Output: $(EXE)"

prepare:
	@$(MKDIR)

# =============================================
# Shared library builds
# =============================================

shared: shared-$(OS_NAME)

# -------- Windows (DLL + LIB) -------------
shared-Windows:
	@echo "Building shared library for Windows..."
	clang-cl /c $(INC_DIR)\libjsonval.cpp /Fo:libjsonval.obj
	llvm-lib libjsonval.obj /OUT:$(LIB_DIR)\libjsonval.lib
	clang-cl /LD $(INC_DIR)\libjsonval.cpp /link /OUT:$(SHARED_LIB)
	@echo "Created DLL: $(SHARED_LIB)"

# -------- Linux (.so) ----------------------
shared-Linux:
	@echo "Building shared library for Linux..."
	$(CXX) -shared -fPIC $(INC_DIR)/libjsonval.cpp -o $(SHARED_LIB)
	@echo "Created SO: $(SHARED_LIB)"

# -------- macOS (.dylib) -------------------
shared-Darwin:
	@echo "Building shared library for macOS..."
	$(CXX) -dynamiclib $(INC_DIR)/libjsonval.cpp -o $(SHARED_LIB)
	@echo "Created DYLIB: $(SHARED_LIB)"

# =============================================
# Clean
# =============================================
clean:
ifeq ($(OS_NAME),Windows)
	@echo "Cleaning Windows build..."
	@if exist $(BUILD)\bvald.exe del /Q $(BUILD)\bvald.exe
	@if exist $(LIB_DIR)\libjsonval.dll del /Q $(LIB_DIR)\libjsonval.dll
	@if exist $(LIB_DIR)\libjsonval.lib del /Q $(LIB_DIR)\libjsonval.lib
	@if exist libjsonval.obj del /Q libjsonval.obj
else
	@echo "Cleaning $(OS_NAME) build..."
	rm -rf $(BUILD)/bvald $(SHARED_LIB) libjsonval.obj $(LIB_DIR)/libjsonval.*
endif
