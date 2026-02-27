CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

PROJECT_ROOT := ../..

MCU     = -mcpu=cortex-m4 -mthumb
DEFS    = -DSTM32F411xE

CFLAGS  = $(MCU) -O0 -g -Wall -ffreestanding $(DEFS) -u _printf_float
LDFLAGS = -T $(PROJECT_ROOT)/lib/linker.ld -nostdlib -lc -lm -lgcc -lnosys

COMMON_DIR=../common
COMMON= \
 $(COMMON_DIR)/usart_debug.c \
 $(COMMON_DIR)/spi2.c \
 $(COMMON_DIR)/w5500.c \
 $(COMMON_DIR)/delay.c \
 $(COMMON_DIR)/system_stm32f4xx.c \
 $(COMMON_DIR)/startup.s \
 $(COMMON_DIR)/syscalls.c \

INCLUDES = \
 -I$(PROJECT_ROOT)/include/cmsis/Include \
 -I$(PROJECT_ROOT)/include \

SRC = \
 spi.c \
 gps.c \
 sd.c \
 main.c 

OBJ = $(SRC:.c=.o) 
OBJ := $(COMMON:.c=.o)
OBJ := $(OBJ:.s=.o) 
OBJ := $(COMMON:.s=.o)

TARGET = firmware.elf
BIN    = firmware.bin

all: $(TARGET) $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@

$(BIN): $(TARGET)
	$(OBJCOPY) -O binary $< $@

flash:
	openocd -f board/st_nucleo_f4.cfg \
	        -c "program $(TARGET) verify reset exit"

clean:
	rm -f *.o *.elf *.bin ../common/*.o

seriallist:
	ls /dev/ttyACM*

serialmonitor:
	minicom -b 115200 -D /dev/ttyACM0

serialmonitor0:
	minicom -b 115200 -D /dev/ttyACM0

serialmonitor1:
	minicom -b 115200 -D /dev/ttyACM1

.PHONY: all clean flash serialmonitor
	
