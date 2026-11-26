/* Build-time constants. Button codes and repeats live in DataFlash settings (src/settings.h,
 * docs/ble_protocol.md) and are written over BLE. */
#ifndef CONFIG_H
#define CONFIG_H
#define DEBOUNCE_MS      20
#define CONFIG_HOLD_MS   4000   /* both buttons this long -> BLE configuration mode */
#define IR_CARRIER_HZ    38000

/* Firmware version reported by the BLE `status` characteristic. */
#define FW_VERSION_MAJOR 0
#define FW_VERSION_MINOR 2
#define FW_VERSION_PATCH 0

/* make TEST_CODES=1: on a clean chip (no valid settings in flash) use these codes instead of
 * "not assigned", so the IR checks of add-ir-nec-transmit can be done before BLE works.
 * Nothing is written to flash. Test values from the ir-transmit spec scenario. */
#define TEST_CODE_A_ADDRESS 0x32
#define TEST_CODE_A_COMMAND 0x02
#define TEST_CODE_B_ADDRESS 0x32
#define TEST_CODE_B_COMMAND 0x03
#endif
