# COMPILE_FLAGS = -g -Wall -Wextra -Wpedantic -Weffc++ -Og -std=c++11 -Wfatal-errors -D_FILE_OFFSET_BITS=64
COMPILE_FLAGS = -O2 -Wall -Wextra -Wpedantic -Weffc++ -std=c++11 -Wfatal-errors -D_FILE_OFFSET_BITS=64
LINK_FLAGS = -lfuse3 -lpthread -lX11

SOURCE_DIR = src
BUILD_DIR = build
SOURCES = $(wildcard $(SOURCE_DIR)/*.cpp)
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SOURCES:.cpp=.o)))
TARGET = a.out

$(TARGET): $(OBJECTS)
	g++ $(LINK_FLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	g++ $(COMPILE_FLAGS) -c -o $@ $<

.PHONY: clean

clean:
	@rm -f $(TARGET) $(OBJECTS) core
