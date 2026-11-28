# Target build for WCH CH592F with xPack riscv-none-elf-gcc (no MounRiver Studio).
#   make                      -> build/ulpanir/remote.{elf,hex,bin}
#   make BOARD=weact APP=blink
#   make LOG=0                -> without the UART log (src/log.h)
#   make TEST_CODES=1          -> clean chip uses test codes 0x32/0x02, 0x32/0x03 (IR checks before BLE)
#   make test                 -> host unit tests (no hardware)
#   make flash-isp            -> USB bootloader (wchisp), board held in BOOT
#   make flash-link           -> WCH-LinkE over SWD (wlink)
#   make stand-flash          -> test stand on ESP32-C6 (../stand), RVSWD through header J1
#   make stand                -> stand: flash, erase settings, run the board checks (GOOD / REJECT)
TOOLCHAIN ?= $(HOME)/tools/xpack-riscv-none-elf-gcc-15.2.0-1/bin/riscv-none-elf-
WCHISP    ?= $(HOME)/tools/bin/wchisp-macos-arm64/wchisp
WLINK     ?= $(HOME)/tools/bin/wlink-macos-arm64/wlink
BOARD     ?= ulpanir
APP       ?= remote
TEST_CODES ?= 0
LOG       ?= 1

CC      = $(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy
SIZE    = $(TOOLCHAIN)size

SDK = vendor/ch592
BUILD = build/$(BOARD)$(if $(filter 1,$(TEST_CODES)),-testcodes)$(if $(filter 0,$(LOG)),-nolog)
LOG_DEF = $(if $(filter 1,$(LOG)),-DDEBUG=Debug_UART1)   # printf -> UART1 TX (PA9), src/log.h
BOARD_DEF = $(if $(filter ulpanir,$(BOARD)),BOARD_ULPANIR,$(if $(filter weact,$(BOARD)),BOARD_WEACT,$(error BOARD must be ulpanir or weact)))

BLE = $(SDK)/BLE
BLE_INC = -I$(BLE)/LIB -I$(BLE)/HAL/include
BLE_DEFS = -DBLE_SNV=0 -DCLK_OSC32K=1   # no bonding storage; internal 32 kHz RC (no 32 kHz crystal on our PCB)

ARCH    = -march=rv32imac_zicsr_zifencei -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore
CFLAGS  = $(ARCH) -Os -std=gnu11 -Wall -Wextra -Wno-unused-parameter -ffunction-sections -fdata-sections -fno-common \
          -D$(BOARD_DEF) -DTEST_CODES=$(TEST_CODES) $(LOG_DEF) -DINT_SOFT $(BLE_DEFS) -Isrc -I$(SDK)/StdPeriphDriver/inc -I$(SDK)/RVMSIS $(BLE_INC)
SDKFLAGS = $(ARCH) -Os -std=gnu11 -w -ffunction-sections -fdata-sections -fno-common -DINT_SOFT $(LOG_DEF) $(BLE_DEFS) \
          -I$(SDK)/StdPeriphDriver/inc -I$(SDK)/RVMSIS $(BLE_INC)
LDFLAGS = $(ARCH) -T $(SDK)/Ld/Link.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(APP).map \
          --specs=nano.specs --specs=nosys.specs -L$(SDK)/StdPeriphDriver -Wl,--print-memory-usage

ifeq ($(APP),remote)
APP_SRC = src/main.c src/nec.c src/remote.c src/ir_timing.c src/ir_tx.c src/settings.c src/settings_store.c src/dispatch.c src/proto.c src/ble_config.c src/log.c src/battery.c src/power.c
else ifeq ($(APP),blink)
APP_SRC = src/blink.c
else
$(error APP must be remote or blink)
endif
SDK_SRC = $(SDK)/StdPeriphDriver/CH59x_clk.c $(SDK)/StdPeriphDriver/CH59x_gpio.c $(SDK)/StdPeriphDriver/CH59x_sys.c \
          $(SDK)/StdPeriphDriver/CH59x_timer0.c $(SDK)/StdPeriphDriver/CH59x_timer1.c $(SDK)/StdPeriphDriver/CH59x_pwr.c \
          $(SDK)/RVMSIS/core_riscv.c
ifeq ($(APP),remote)
SDK_SRC += $(SDK)/StdPeriphDriver/CH59x_adc.c $(SDK)/StdPeriphDriver/CH59x_flash.c $(SDK)/StdPeriphDriver/CH59x_uart1.c \
           $(BLE)/HAL/MCU.c $(BLE)/HAL/RTC.c $(BLE)/HAL/SLEEP.c $(BLE)/HAL/LED.c $(BLE)/HAL/KEY.c
endif

APP_OBJ = $(patsubst src/%.c,$(BUILD)/$(APP)/%.o,$(APP_SRC))
SDK_OBJ = $(patsubst $(SDK)/%.c,$(BUILD)/sdk/%.o,$(SDK_SRC)) $(BUILD)/sdk/startup.o
ifeq ($(APP),remote)
SDK_OBJ += $(BUILD)/sdk/ble_task_scheduler.o
LIBS = -L$(BLE)/LIB -lCH59xBLE -lISP592
else
LIBS = -lISP592
endif
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
$(BUILD)/sdk/ble_task_scheduler.o: $(BLE)/LIB/ble_task_scheduler.S
	@mkdir -p $(dir $@)
	$(CC) $(ARCH) -x assembler-with-cpp -c $< -o $@
$(OUT).elf: $(APP_OBJ) $(SDK_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)
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

STAND = uv run ../stand/host/stand.py
stand-flash: $(OUT).bin
	$(STAND) flash $(OUT).bin
stand: $(OUT).bin
	$(STAND) run $(OUT).bin

clean:
	rm -rf build
	$(MAKE) -C test clean
.PHONY: all test flash-isp flash-link stand-flash stand clean
