TOOL_DIR	:= $(HOME)/tools/esp8266
XTENSA		:= $(TOOL_DIR)/compiler/crosstool-NG/builds/xtensa-lx106-elf/bin
SDK_VERSION := 
SDK_BASE 	:= $(TOOL_DIR)/sdk/ESP8266_NONOS_SDK$(SDK_VERSION)
SDK_LIBS 	:= -lc -lgcc -lhal -lphy -lpp -lnet80211 -lwpa -lmain -llwip -lcrypto -ljson
CC 			:= $(XTENSA)/xtensa-lx106-elf-gcc
LD			:= $(XTENSA)/xtensa-lx106-elf-gcc
AR			:= $(XTENSA)/xtensa-lx106-elf-ar
LDFLAGS		:= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static
CFLAGS 		:= -g -Wpointer-arith -Wundef -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -ffunction-sections -fdata-sections -fno-builtin-printf -DICACHE_FLASH -I. -I$(SDK_BASE)/include
LD_SCRIPT	:= -T$(SDK_BASE)/ld/eagle.app.v6.ld
ESPTOOL 	:= $(TOOL_DIR)/esptool/esptool.py

DEVICE_PORT	:= /dev/ttyUSB0
DEVICE_BAUD := 921600

all: main.bin

main.bin: main.out
	$(ESPTOOL) elf2image main.out -o main

main.out: main.a
	$(LD) -L$(SDK_BASE)/lib $(LD_SCRIPT) $(LDFLAGS) -L$(SDK_BASE)/lib -Wl,--start-group $(SDK_LIBS) main.a -Wl,--end-group -o main.out

main.a: main.o rf_init.o
	$(AR) cru main.a main.o rf_init.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o *.bin *.a *.out

flash: main.bin
	$(ESPTOOL) --port $(DEVICE_PORT) --baud $(DEVICE_BAUD) write_flash --erase-all --flash_size detect 0x00000 main0x00000.bin 0x10000 main0x10000.bin 0x200000 light.html 0x3fc000 $(SDK_BASE)/bin/esp_init_data_default.bin
#	$(ESPTOOL) --port $(DEVICE_PORT) --baud $(DEVICE_BAUD) write_flash --flash_size detect 0x00000 main0x00000.bin 0x10000 main0x10000.bin 0x200000 light.html 0x3fc000 $(SDK_BASE)/bin/esp_init_data_default.bin

.PHONY: all clean flash
