# Compiler and flags
# Assumes headers are in 'headers' directory and Boost libraries are installed.
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Iheaders
LDFLAGS := -lpthread -lboost_system -lboost_thread

# Directories and target
SRC_DIR := source
BUILD_DIR := build
TARGET := p2p

# List of all .cpp sources in source directory and subdirectories
SRCS := $(wildcard $(SRC_DIR)/*.cpp) \
        $(wildcard $(SRC_DIR)/log/*.cpp) \
        $(wildcard $(SRC_DIR)/message/*.cpp) \
        $(wildcard $(SRC_DIR)/network/*.cpp) \
        $(wildcard $(SRC_DIR)/ui/*.cpp)

# Map .cpp files to .o files in build directory
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Default target: build the executable
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile .cpp files to .o files, preserving directory structure
# Header dependencies are not tracked; use 'make clean' if headers change
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts and executable
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean