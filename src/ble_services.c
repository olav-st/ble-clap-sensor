#include <zephyr.h>
#include <logging/log.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <clap_detection.h>

LOG_MODULE_REGISTER(ble_services);

/* Nordic LED Button Service  */
/* See: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/include/bluetooth/services/lbs.html */
static struct bt_uuid_128 led_btn_service_uuid = BT_UUID_INIT_128(
    0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15,
    0xDE, 0xEF, 0x12, 0x12, 0x23, 0x15, 0x00, 0x00);

/* Button Characteristic */
static struct bt_uuid_128 btn_char_uuid = BT_UUID_INIT_128(
    0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15,
    0xDE, 0xEF, 0x12, 0x12, 0x24, 0x15, 0x00, 0x00);

/* Sensitivity Characteristic */
static struct bt_uuid_128 sensitivity_char_uuid = BT_UUID_INIT_128(
    0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15,
    0xDE, 0xEF, 0x12, 0x12, 0x26, 0x15, 0x00, 0x00);

/* Advertising data */
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* BLE connection */
struct bt_conn *conn;
/* Notification state */
volatile bool notify_enable;
/* Button value */
uint8_t btn_state = 0;

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    ARG_UNUSED(attr);
    notify_enable = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Notification %s", notify_enable ? "enabled" : "disabled");
}

static ssize_t read_btn_state(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &btn_state, sizeof(btn_state));
}

static ssize_t write_treshold(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    LOG_INF("Received new treshold value");
    int treshold = 0;
    memcpy(&treshold, buf, sizeof(treshold));
    clap_detection_set_treshold(treshold);
    return 0;
}

/* GATT services and characteristics */
BT_GATT_SERVICE_DEFINE(
    ble_services,
    BT_GATT_PRIMARY_SERVICE(&led_btn_service_uuid),
    BT_GATT_CHARACTERISTIC(&btn_char_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, read_btn_state, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(&sensitivity_char_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE , BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, write_treshold, (void *)1),
);

static void bt_ready(int err)
{
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }
    LOG_INF("Bluetooth initialized");
    /* Start advertising */
    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    LOG_INF("Configuration mode: waiting connections...");
}

static void connected(struct bt_conn *connected, uint8_t err)
{
    if (err)
    {
        LOG_ERR("Connection failed (err %u)", err);
    }
    else
    {
        LOG_INF("Connected");
        if (!conn)
        {
            conn = bt_conn_ref(connected);
        }
    }
}

static void disconnected(struct bt_conn *disconn, uint8_t reason)
{
    if (conn)
    {
        bt_conn_unref(conn);
        conn = NULL;
    }

    LOG_INF("Disconnected (reason %u)", reason);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

void ble_services_set_button_state(bool state)
{
    int err;

    LOG_INF("Setting button state to %d", state);
    btn_state = state;
    if (conn)
    {
        if (notify_enable)
        {
            err = bt_gatt_notify(NULL, &ble_services.attrs[1],
                         &btn_state, sizeof(btn_state));
            if (err)
            {
                LOG_ERR("Notify error: %d", err);
            }
            else 
            {
                LOG_INF("Send notify ok (val: %d)", btn_state);
            }
        } else 
        {
            LOG_INF("Notify not enabled");
        }
    } else 
    {
        LOG_INF("BLE not connected");
    }
}

int ble_services_init(void)
{
    // Initialize the Bluetooth Subsystem 
    bt_conn_cb_register(&conn_callbacks);
    int err = bt_enable(bt_ready);
    return err;
}