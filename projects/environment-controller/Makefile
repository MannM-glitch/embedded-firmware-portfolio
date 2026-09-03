CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS := -Isrc
BUILD_DIR := build
FIRMWARE_SOURCES := src/controller.c src/hardware_sim.c

.PHONY: all run test clean

all: $(BUILD_DIR)/environment-controller $(BUILD_DIR)/controller-tests

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/environment-controller: $(FIRMWARE_SOURCES) src/main.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FIRMWARE_SOURCES) src/main.c -o $@

$(BUILD_DIR)/controller-tests: $(FIRMWARE_SOURCES) tests/test_controller.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FIRMWARE_SOURCES) tests/test_controller.c -o $@

run: $(BUILD_DIR)/environment-controller
	./$(BUILD_DIR)/environment-controller

test: $(BUILD_DIR)/controller-tests
	./$(BUILD_DIR)/controller-tests

clean:
	rm -rf $(BUILD_DIR)
