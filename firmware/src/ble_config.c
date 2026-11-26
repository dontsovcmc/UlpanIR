/* BLE configuration mode (add-remote-config): GATT service of docs/ble_protocol.md on the WCH BLE stack.
 * Entered from main when both buttons are held 4 s; never returns: on 60 s without commands the remote
 * switches off (our board: power latch released) or resets (dev board: advertising stops, radio off). */
#include "ble_config.h"
#include "HAL.h"
#include "board.h"
#include "ir_tx.h"
#include "nec.h"
#include "proto.h"
#include "settings_store.h"

#define EVT_START   0x0001
#define EVT_BLINK   0x0002
#define EVT_TIMEOUT 0x0004

#define TIMEOUT_MS  60000
#define BLINK_MS    250
#define ADV_INTERVAL 160 /* 100 ms in 0.625 ms units */

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4]; /* BLE stack heap, used by HAL/MCU.c */

static tmosTaskID task_id = INVALID_TASK_ID;
static settings_t *work;
static uint16_t battery_mv;
static bool led_on;
static nec_frame_t test_frame;

/* ---- GATT service ---- */
static uint8_t uuid_service[ATT_UUID_SIZE], uuid_config[ATT_UUID_SIZE], uuid_status[ATT_UUID_SIZE],
    uuid_ping[ATT_UUID_SIZE], uuid_test[ATT_UUID_SIZE];
static gattAttrType_t service_type = {ATT_UUID_SIZE, uuid_service};

static uint8_t props_config = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8_t props_status = GATT_PROP_READ;
static uint8_t props_ping = GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
static uint8_t props_test = GATT_PROP_WRITE;
static uint8_t value_dummy[1]; /* values are produced in the callbacks */

static gattAttribute_t attrs[] = {
    {{ATT_BT_UUID_SIZE, primaryServiceUUID}, GATT_PERMIT_READ, 0, (uint8_t *)&service_type},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &props_config},
    {{ATT_UUID_SIZE, uuid_config}, GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, value_dummy},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &props_status},
    {{ATT_UUID_SIZE, uuid_status}, GATT_PERMIT_READ, 0, value_dummy},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &props_ping},
    {{ATT_UUID_SIZE, uuid_ping}, GATT_PERMIT_WRITE, 0, value_dummy},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &props_test},
    {{ATT_UUID_SIZE, uuid_test}, GATT_PERMIT_WRITE, 0, value_dummy},
};

static bool is_uuid(const gattAttribute_t *a, const uint8_t *u) {
    return a->type.len == ATT_UUID_SIZE && tmos_memcmp(a->type.uuid, u, ATT_UUID_SIZE);
}

/* Any command restarts the 60 s countdown (spec: "Тайм-аут режима настройки"). */
static void touch(void) {
    tmos_stop_task(task_id, EVT_TIMEOUT);
    tmos_start_task(task_id, EVT_TIMEOUT, MS1_TO_SYSTEM_TIME(TIMEOUT_MS));
}

static bStatus_t read_cb(uint16_t conn, gattAttribute_t *a, uint8_t *val, uint16_t *len, uint16_t offset,
                         uint16_t max_len, uint8_t method) {
    (void)conn;
    (void)method;
    touch();
    if (offset > 0) return ATT_ERR_ATTR_NOT_LONG;
    if (is_uuid(a, uuid_config) && max_len >= SETTINGS_BLOCK_LEN) {
        proto_config_read(work, val);
        *len = SETTINGS_BLOCK_LEN;
        return SUCCESS;
    }
    if (is_uuid(a, uuid_status) && max_len >= PROTO_STATUS_LEN) {
        proto_status(battery_mv, val);
        *len = PROTO_STATUS_LEN;
        return SUCCESS;
    }
    *len = 0;
    return ATT_ERR_ATTR_NOT_FOUND;
}

static bStatus_t write_cb(uint16_t conn, gattAttribute_t *a, uint8_t *val, uint16_t len, uint16_t offset,
                          uint8_t method) {
    (void)conn;
    (void)method;
    touch();
    if (offset > 0) return ATT_ERR_ATTR_NOT_LONG;
    if (is_uuid(a, uuid_ping)) return SUCCESS;
    if (is_uuid(a, uuid_config)) return (bStatus_t)proto_config_write(val, len, work, settings_save);
    if (is_uuid(a, uuid_test)) {
        button_code_t code;
        int err = proto_test_write(val, len, work, &code);
        if (err != SETTINGS_OK) return (bStatus_t)err;
        if (ir_tx_busy()) return SETTINGS_ERR_BUSY;
        nec_encode(code.address, code.command, code.extended, &test_frame);
        ir_tx_start(&test_frame);
        return SUCCESS;
    }
    return ATT_ERR_ATTR_NOT_FOUND;
}

static gattServiceCBs_t service_cbs = {read_cb, write_cb, NULL};

/* ---- advertising ---- */
static uint8_t adv_data[3 + 2 + ATT_UUID_SIZE];
static uint8_t scan_rsp[2 + PROTO_NAME_LEN];
static uint8_t device_name[GAP_DEVICE_NAME_LEN];

static void build_adv(void) {
    uint8_t mac[6];
    char name[PROTO_NAME_LEN + 1];
    GetMACAddress(mac); /* hypothesis: least significant byte first, as BLE addresses are stored */
    proto_adv_name(mac, name);

    adv_data[0] = 2;
    adv_data[1] = GAP_ADTYPE_FLAGS;
    adv_data[2] = GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;
    adv_data[3] = 1 + ATT_UUID_SIZE;
    adv_data[4] = GAP_ADTYPE_128BIT_COMPLETE;
    tmos_memcpy(adv_data + 5, uuid_service, ATT_UUID_SIZE);

    scan_rsp[0] = 1 + PROTO_NAME_LEN;
    scan_rsp[1] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    tmos_memcpy(scan_rsp + 2, name, PROTO_NAME_LEN);

    tmos_memset(device_name, 0, sizeof(device_name));
    tmos_memcpy(device_name, name, PROTO_NAME_LEN);
}

static void set_advertising(uint8_t on) { GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(on), &on); }

static void state_cb(gapRole_States_t state, gapRoleEvent_t *ev) {
    (void)ev;
    /* after a disconnect, advertise again until the timeout switches the remote off */
    if ((state & GAPROLE_STATE_ADV_MASK) == GAPROLE_WAITING) set_advertising(TRUE);
}

static gapRolesCBs_t role_cbs = {state_cb, NULL, NULL};
static gapBondCBs_t bond_cbs = {NULL, NULL, NULL};

/* ---- leaving the mode ---- */
static void switch_off(void) {
    set_advertising(FALSE);
    led_set(0);
#if HAS_HOLD
    GPIOA_ResetBits(HOLD_PIN); /* power latch: the board loses power here */
    mDelaymS(50);
#endif
    SYS_ResetExecute(); /* still powered (dev board, programmer): restart in normal mode, radio off */
}

static tmosEvents process_event(tmosTaskID id, tmosEvents events) {
    if (events & SYS_EVENT_MSG) {
        uint8_t *msg = tmos_msg_receive(id);
        if (msg) tmos_msg_deallocate(msg);
        return events ^ SYS_EVENT_MSG;
    }
    if (events & EVT_START) {
        GAPRole_PeripheralStartDevice(task_id, &bond_cbs, &role_cbs);
        return events ^ EVT_START;
    }
    if (events & EVT_BLINK) {
        if (!ir_tx_busy()) { /* during a test frame the LED shows the frame */
            led_on = !led_on;
            led_set(led_on);
        }
        tmos_start_task(task_id, EVT_BLINK, MS1_TO_SYSTEM_TIME(BLINK_MS));
        return events ^ EVT_BLINK;
    }
    if (events & EVT_TIMEOUT) {
        switch_off();
        return events ^ EVT_TIMEOUT;
    }
    return 0;
}

static uint16_t measure_battery_mv(void) {
    ADC_InterBATSampInit();
    signed short calib = ADC_DataCalib_Rough(); /* restores channel and gain itself */
    (void)ADC_ExcutSingleConver(); /* first conversion is discarded, as in WCH HAL/BAT.c */
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) {
        int v = (int)ADC_ExcutSingleConver() + calib;
        sum += v > 0 ? (uint32_t)v : 0;
    }
    R8_ADC_CFG &= ~RB_ADC_POWER_ON;
    return proto_vbat_mv((uint16_t)(sum / 8));
}

__HIGH_CODE __attribute__((noinline, noreturn)) static void loop(void) {
    for (;;) TMOS_SystemProcess();
}

void ble_config_run(settings_t *settings) {
    work = settings;
    battery_mv = measure_battery_mv(); /* before the radio starts: no RF load on the battery */

    proto_uuid(PROTO_UUID_SERVICE, uuid_service);
    proto_uuid(PROTO_UUID_CONFIG, uuid_config);
    proto_uuid(PROTO_UUID_STATUS, uuid_status);
    proto_uuid(PROTO_UUID_PING, uuid_ping);
    proto_uuid(PROTO_UUID_TEST, uuid_test);
    build_adv();

    CH59x_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    task_id = TMOS_ProcessEventRegister(process_event);

    uint8_t on = TRUE;
    uint16_t adv_int = ADV_INTERVAL;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(on), &on);
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(adv_data), adv_data);
    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scan_rsp), scan_rsp);
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, adv_int);
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, adv_int);

    uint8_t pair_mode = GAPBOND_PAIRING_MODE_NO_PAIRING, bonding = FALSE;
    GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(pair_mode), &pair_mode);
    GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(bonding), &bonding);

    GGS_AddService(GATT_ALL_SERVICES);
    GATTServApp_AddService(GATT_ALL_SERVICES);
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, device_name);
    GATTServApp_RegisterService(attrs, GATT_NUM_ATTRS(attrs), GATT_MAX_ENCRYPT_KEY_SIZE, &service_cbs);

    tmos_set_event(task_id, EVT_START);
    tmos_start_task(task_id, EVT_BLINK, MS1_TO_SYSTEM_TIME(BLINK_MS));
    touch();
    loop();
}
