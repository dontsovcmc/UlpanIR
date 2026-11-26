#include "settings_store.h"
#include "CH59x_common.h"

/* Each slot in its own 4 KB erase block: on a chip reporting DEF_CHIP_ID_CH592A (QFN-28) the SDK hangs on
 * erases that are not a multiple of 4 KB (ISP592.h). Hypothesis to check on the board: StartAddr of EEPROM_*
 * is an offset from the start of DataFlash. */
#define SLOT_OFFSET(i) ((uint32_t)(i) * EEPROM_BLOCK_SIZE)
#define SLOT_WORDS     ((SETTINGS_SLOT_LEN + 3) / 4) /* SDK buffers: RAM, 4-byte aligned */

static int best = -1;
static uint32_t counter;

static void read_slot(int i, uint32_t buf[SLOT_WORDS]) { EEPROM_READ(SLOT_OFFSET(i), buf, SETTINGS_SLOT_LEN); }

void settings_load(settings_t *s) {
    uint32_t s0[SLOT_WORDS], s1[SLOT_WORDS];
    read_slot(0, s0);
    read_slot(1, s1);
    best = settings_pick((const uint8_t *)s0, (const uint8_t *)s1, s, &counter);
}

int settings_save(const settings_t *s) {
    int slot = settings_next_slot(best);
    uint32_t buf[SLOT_WORDS] = {0}, check[SLOT_WORDS] = {0};
    settings_slot_pack(counter + 1, s, (uint8_t *)buf);
    if (EEPROM_ERASE(SLOT_OFFSET(slot), EEPROM_BLOCK_SIZE) != 0) return SETTINGS_ERR_FLASH;
    if (EEPROM_WRITE(SLOT_OFFSET(slot), buf, SETTINGS_SLOT_LEN) != 0) return SETTINGS_ERR_FLASH;
    read_slot(slot, check);
    if (memcmp(buf, check, SETTINGS_SLOT_LEN) != 0) return SETTINGS_ERR_FLASH;
    best = slot;
    counter++;
    return SETTINGS_OK;
}
