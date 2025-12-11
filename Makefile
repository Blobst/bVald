# =============================================
# Compiler and directories
# =============================================
CXX = clang++
CXXFLAGS = -std=c++23 -I$(INC_DIR)

LIB_DIR = lib
INC_DIR = include
SRC_DIR = src
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
	@echo "Building app for $(OS_NAME)..."
	$(CXX) $(CXXFLAGS) -DJSONVAL_EXPORTS $(SRC) $(INC_DIR)/libjsonval.cpp $(INC_DIR)/jls.cpp $(INC_DIR)/jls_shell.cpp $(INC_DIR)/jls_library.cpp -o $(EXE)
	@echo "Output: $(EXE)"

prepare:
	@$(MKDIR)

# =============================================
# Shared library builds
# =============================================

shared: shared-$(OS_NAME)

# -------- Windows (DLL + import lib) ---------
shared-Windows:
	@echo "Building JSONVAL DLL for Windows..."
	$(CXX) -DJSONVAL_EXPORTS -shared $(INC_DIR)/libjsonval.cpp -I$(INC_DIR) -o $(SHARED_LIB) -Wl,--out-implib=$(LIB_DIR)/libjsonval.a
	@echo "Created DLL: $(SHARED_LIB)"
	@echo "Import Library: $(LIB_DIR)/libjsonval.a"

# -------- Linux (.so) -------------------------
shared-Linux:
	@echo "Building JSONVAL .so..."
	$(CXX) -DJSONVAL_EXPORTS -shared -fPIC $(INC_DIR)/libjsonval.cpp -I$(INC_DIR) -o $(SHARED_LIB)
	@echo "Created SO: $(SHARED_LIB)"

# -------- macOS (.dylib) ----------------------
shared-Darwin:
	@echo "Building JSONVAL .dylib..."
	$(CXX) -DJSONVAL_EXPORTS -dynamiclib $(INC_DIR)/libjsonval.cpp -I$(INC_DIR) -o $(SHARED_LIB)
	@echo "Created DYLIB: $(SHARED_LIB)"

# =============================================
# Clean
# =============================================
clean:
ifeq ($(OS_NAME),Windows)
	@echo "Cleaning Windows build..."
	@if exist $(BUILD)\bvald.exe del /Q $(BUILD)\bvald.exe
	@if exist $(LIB_DIR)\libjsonval.dll del /Q $(LIB_DIR)\libjsonval.dll
	@if exist $(LIB_DIR)\libjsonval.a del /Q $(LIB_DIR)\libjsonval.a
else
	@echo "Cleaning $(OS_NAME) build..."
	rm -rf $(BUILD)/bvald $(LIB_DIR)/libjsonval.* 
endif
