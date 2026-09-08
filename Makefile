CC := gcc
CFLAGS := -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -std=c11 -O2 -Iinclude -Iservices
LDFLAGS :=

SUPERVISOR_OBJS := src/main.o src/supervisor.o src/service.o src/signal_handler.o src/logger.o
SERVICES := camera sensor object_detection lane_detection
SERVICE_BINS := $(SERVICES:%=bin/%_service)

.PHONY: all clean run test dirs

all: dirs bin/adas_supervisor $(SERVICE_BINS)

bin/adas_supervisor: $(SUPERVISOR_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bin/camera_service: services/camera_service.o services/service_common.o
	$(CC) $(CFLAGS) -o $@ $^

bin/sensor_service: services/sensor_service.o services/service_common.o
	$(CC) $(CFLAGS) -o $@ $^

bin/object_detection_service: services/object_detection_service.o services/service_common.o
	$(CC) $(CFLAGS) -o $@ $^

bin/lane_detection_service: services/lane_detection_service.o services/service_common.o
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

services/%.o: services/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

dirs:
	@mkdir -p bin logs

run: all
	./bin/adas_supervisor config/services.conf logs/supervisor.log

test: all
	./tests/test_config.sh

clean:
	rm -f src/*.o services/*.o bin/*
