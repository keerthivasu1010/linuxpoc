CC       := gcc
CFLAGS   := -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -g
SRC_DIR  := src
BIN_DIR  := bin
TARGET   := $(BIN_DIR)/adas_supervisor

SOURCES  := $(wildcard $(SRC_DIR)/*.c)
OBJECTS  := $(SOURCES:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

run: all
	./$(TARGET)

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
	rm -rf $(BIN_DIR)
