# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iheaders
LDFLAGS = -lboost_asio -pthread

# Directories
SRC_DIR = source
HDR_DIR = headers
BLD_DIR = build
BIN_DIR = $(BLD_DIR)/bin
TEST_DIR = tests

# All source files and headers are found recursively
SOURCES = $(shell find $(SRC_DIR) -name '*.cpp')
HEADERS = $(shell find $(HDR_DIR) -name '*.h')
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BLD_DIR)/%.o)

# Output binary
TARGET = $(BIN_DIR)/p2p_file_sharing

# Default target
all: $(TARGET)

# Linking object files to create the executable
$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compiling source files to object files
$(BLD_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Cleaning build directory
clean:
	rm -rf $(BLD_DIR)

# Running the program
run: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: all clean run
