CXX := g++
CXXFLAGS := -std=c++17 -Wall -Iheaders
LDFLAGS := -lpthread -lboost_system -lboost_thread

SRC_DIR := source
BUILD_DIR := build
TARGET := p2p

# List of all .cpp sources
SRCS := $(wildcard $(SRC_DIR)/*.cpp) \
        $(wildcard $(SRC_DIR)/log/*.cpp) \
        $(wildcard $(SRC_DIR)/message/*.cpp) \
        $(wildcard $(SRC_DIR)/network/*.cpp) \
        $(wildcard $(SRC_DIR)/ui/*.cpp)

# Map .cpp → build/.o
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Default rule
all: $(TARGET)

# Link executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile .cpp → .o (mirror folder structure under build/)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean

