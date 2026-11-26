# Target build for WCH CH592F with xPack riscv-none-elf-gcc (no MounRiver Studio).
#   make                      -> build/ulpanir/remote.{elf,hex,bin}
#   make BOARD=weact APP=blink
#   make test                 -> host unit tests (no hardware)
#   make flash-isp            -> USB bootloader (wchisp), board held in BOOT
#   make flash-link           -> WCH-LinkE over SWD (wlink)
TOOLCHAIN ?= $(HOME)/tools/xpack-riscv-none-elf-gcc-15.2.0-1/bin/riscv-none-elf-
WCHISP    ?= $(HOME)/tools/bin/wchisp-macos-arm64/wchisp
WLINK     ?= $(HOME)/tools/bin/wlink-macos-arm64/wlink
BOARD     ?= ulpanir
APP       ?= remote

CC      = $(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy
SIZE    = $(TOOLCHAIN)size

SDK = vendor/ch592
BUILD = build/$(BOARD)
BOARD_DEF = $(if $(filter ulpanir,$(BOARD)),BOARD_ULPANIR,$(if $(filter weact,$(BOARD)),BOARD_WEACT,$(error BOARD must be ulpanir or weact)))

ARCH    = -march=rv32imac_zicsr_zifencei -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore
CFLAGS  = $(ARCH) -Os -std=gnu11 -Wall -Wextra -Wno-unused-parameter -ffunction-sections -fdata-sections -fno-common \
          -D$(BOARD_DEF) -DINT_SOFT -Isrc -I$(SDK)/StdPeriphDriver/inc -I$(SDK)/RVMSIS
SDKFLAGS = $(ARCH) -Os -std=gnu11 -w -ffunction-sections -fdata-sections -fno-common -DINT_SOFT \
          -I$(SDK)/StdPeriphDriver/inc -I$(SDK)/RVMSIS
LDFLAGS = $(ARCH) -T $(SDK)/Ld/Link.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(APP).map \
          --specs=nano.specs --specs=nosys.specs -L$(SDK)/StdPeriphDriver -lISP592

ifeq ($(APP),remote)
APP_SRC = src/main.c src/nec.c src/remote.c src/ir_timing.c src/ir_tx.c
else ifeq ($(APP),blink)
APP_SRC = src/blink.c
else
$(error APP must be remote or blink)
endif
SDK_SRC = $(SDK)/StdPeriphDriver/CH59x_clk.c $(SDK)/StdPeriphDriver/CH59x_gpio.c $(SDK)/StdPeriphDriver/CH59x_sys.c \
          $(SDK)/StdPeriphDriver/CH59x_timer0.c $(SDK)/StdPeriphDriver/CH59x_timer1.c $(SDK)/StdPeriphDriver/CH59x_pwr.c \
          $(SDK)/RVMSIS/core_riscv.c

APP_OBJ = $(patsubst src/%.c,$(BUILD)/$(APP)/%.o,$(APP_SRC))
SDK_OBJ = $(patsubst $(SDK)/%.c,$(BUILD)/sdk/%.o,$(SDK_SRC)) $(BUILD)/sdk/startup.o
OUT = $(BUILD)/$(APP)

all: $(OUT).hex $(OUT).bin
	@$(SIZE) $(OUT).elf

$(BUILD)/$(APP)/%.o: src/%.c $(wildcard src/*.h) Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/sdk/%.o: $(SDK)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(SDKFLAGS) -c $< -o $@
$(BUILD)/sdk/startup.o: $(SDK)/Startup/startup_CH592.S
	@mkdir -p $(dir $@)
	$(CC) $(ARCH) -x assembler-with-cpp -c $< -o $@
$(OUT).elf: $(APP_OBJ) $(SDK_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ -lISP592
$(OUT).hex: $(OUT).elf
	$(OBJCOPY) -O ihex $< $@
$(OUT).bin: $(OUT).elf
	$(OBJCOPY) -O binary $< $@

test:
	$(MAKE) -C test

flash-isp: $(OUT).bin
	$(WCHISP) flash $(OUT).bin
flash-link: $(OUT).bin
	$(WLINK) flash $(OUT).bin

clean:
	rm -rf build
	$(MAKE) -C test clean
.PHONY: all test flash-isp flash-link clean
