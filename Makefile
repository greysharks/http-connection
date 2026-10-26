include sources.mk

TARGET = main

CC = gcc
CPPFLAGS = $(INCLUDES)
CFLAGS = -Wall -Werror -g -MMD -MP
LDFLAGS = -Wl,-Map=$(TARGET).map
LDLIBS = -lm

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

%.i: %.c
	$(CC) $(CPPFLAGS) -E $< -o $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.asm: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -S $< -o $@

.PHONY: all
all: build
	size $(TARGET).out

.PHONY: build
build: $(TARGET).out

.PHONY: compile-all
compile-all: $(OBJS)

$(TARGET).out: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TARGET).asm: build
	objdump -d $(TARGET).out > $(TARGET).asm

.PHONY: clean
clean:
	rm -f $(SRCS_DIR)/*.o $(SRCS_DIR)/*.i $(SRCS_DIR)/*.asm $(SRCS_DIR)/*.d *.out *.map *.asm

.PHONY: rebuild
rebuild: clean build