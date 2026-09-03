CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS := -Isrc
BUILD_DIR := build
ECU_SOURCES := src/can_bus_sim.c src/thermal_ecu.c

.PHONY: all run test clean

all: $(BUILD_DIR)/vehicle-thermal-ecu $(BUILD_DIR)/thermal-ecu-tests

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/vehicle-thermal-ecu: $(ECU_SOURCES) src/main.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ECU_SOURCES) src/main.c -o $@

$(BUILD_DIR)/thermal-ecu-tests: $(ECU_SOURCES) tests/test_thermal_ecu.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ECU_SOURCES) tests/test_thermal_ecu.c -o $@

run: $(BUILD_DIR)/vehicle-thermal-ecu
	./$(BUILD_DIR)/vehicle-thermal-ecu

test: $(BUILD_DIR)/thermal-ecu-tests
	./$(BUILD_DIR)/thermal-ecu-tests

clean:
	rm -rf $(BUILD_DIR)
