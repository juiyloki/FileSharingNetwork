SRC_DIR  := source
OBJ_DIR  := build
BIN      := p2p

# all .cpp files under source
SRCS := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/network/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(OBJS) -o $@ -lboost_system -lpthread

# generic build rule for all .cpp under source
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -std=c++17 -Wall -Wextra -Iheaders -c $< -o $@

# network subdir rule
$(OBJ_DIR)/network/%.o: $(SRC_DIR)/network/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -std=c++17 -Wall -Wextra -Iheaders -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN)

.PHONY: all clean
