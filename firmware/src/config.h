/* Build-time settings until BLE configuration (add-remote-config) exists. */
#ifndef CONFIG_H
#define CONFIG_H
/* Codes: test values from the ir-transmit spec scenario; Optoma X341 codes are not confirmed yet. */
#define CODE_A_ADDRESS   0x32
#define CODE_A_COMMAND   0x02
#define CODE_B_ADDRESS   0x32
#define CODE_B_COMMAND   0x03
#define CODE_EXTENDED    0      /* 1: 16-bit address (extended NEC) */
#define REPEAT_PERIOD_MS 333    /* 3 frames per second while held */
#define REPEAT_NEC_CODE  0      /* 0: repeat the full frame; 1: standard NEC repeat code */
#define DEBOUNCE_MS      20
#define IR_CARRIER_HZ    38000
#endif
