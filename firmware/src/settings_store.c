#include "settings_store.h"
#include "CH59x_common.h"

/* One record at the start of DataFlash. Erase a whole 4 KB block: on a chip reporting DEF_CHIP_ID_CH592A
 * (QFN-28) the SDK hangs on erases that are not a multiple of 4 KB (ISP592.h). StartAddr of EEPROM_* is an
 * offset from the start of DataFlash (as BLE_SNV_ADDR in the SDK CONFIG.h). */
#define RECORD_OFFSET 0
#define RECORD_WORDS  ((SETTINGS_RECORD_LEN + 3) / 4) /* SDK buffers: RAM, 4-byte aligned */

void settings_load(settings_t *s) {
    uint32_t buf[RECORD_WORDS];
    EEPROM_READ(RECORD_OFFSET, buf, SETTINGS_RECORD_LEN);
    settings_record_unpack((const uint8_t *)buf, s);
}

int settings_save(const settings_t *s) {
    uint32_t buf[RECORD_WORDS] = {0};
    settings_record_pack(s, (uint8_t *)buf);
    if (EEPROM_ERASE(RECORD_OFFSET, EEPROM_BLOCK_SIZE) != 0) return SETTINGS_ERR_FLASH;
    if (EEPROM_WRITE(RECORD_OFFSET, buf, SETTINGS_RECORD_LEN) != 0) return SETTINGS_ERR_FLASH;
    return SETTINGS_OK;
}
