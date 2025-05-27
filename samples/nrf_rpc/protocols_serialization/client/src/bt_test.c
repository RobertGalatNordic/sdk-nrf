/*
 * BLE test service with shell advertising control
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_test, CONFIG_NRF_PS_CLIENT_LOG_LEVEL);

// UUID: 12345678-1234-5678-1234-56789abcdef0
#define BT_UUID_CUSTOM_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
#define BT_UUID_CUSTOM_CHAR_VAL \
    BT_UUID_128_ENCODE(0xabcdef01, 0x2345, 0x6789, 0xabcd, 0xef0123456789)

static struct bt_uuid_128 custom_service_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);
static struct bt_uuid_128 custom_char_uuid    = BT_UUID_INIT_128(BT_UUID_CUSTOM_CHAR_VAL);

typedef struct {
    uint32_t value;
    bool notify_enabled;
    bool indicate_enabled;
    struct bt_conn *conn;
} custom_svc_ctx_t;

static custom_svc_ctx_t custom_ctx = {
    .value = 0x1234ABCD,
    .notify_enabled = false,
    .indicate_enabled = false,
    .conn = NULL,
};

static void notify_subscribed(struct bt_conn *conn, void *param);

static ssize_t read_custom_char(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                void *buf, uint16_t len, uint16_t offset)
{
    const uint32_t *value = &custom_ctx.value;
    uint32_t le_val = sys_cpu_to_le32(*value);
    LOG_INF("[BLE] Read: 0x%08X", le_val);
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &le_val, sizeof(le_val));
}

static ssize_t write_custom_char(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    uint32_t *value = &custom_ctx.value;
    if (offset != 0 || len != sizeof(uint32_t)) {
        LOG_ERR("[BLE] Invalid offset or length");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    *value = sys_get_le32(buf);
    LOG_INF("[BLE] Write: 0x%08X", *value);
    notify_subscribed(conn, NULL);
    return len;
}

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    custom_ctx.notify_enabled = (value & BT_GATT_CCC_NOTIFY) != 0;
    custom_ctx.indicate_enabled = (value & BT_GATT_CCC_INDICATE) != 0;
    LOG_INF("[BLE] Notify: %s, Indicate: %s", custom_ctx.notify_enabled ? "ON" : "OFF", custom_ctx.indicate_enabled ? "ON" : "OFF");
}

BT_GATT_SERVICE_DEFINE(custom_svc,
    BT_GATT_PRIMARY_SERVICE(&custom_service_uuid),
    BT_GATT_CHARACTERISTIC(&custom_char_uuid.uuid,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
        read_custom_char, write_custom_char, &custom_ctx.value),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static struct bt_gatt_indicate_params ind_params;

static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
    if (err) {
        LOG_ERR("[BLE] Indication confirmation error: %d", err);
    } else {
        LOG_INF("[BLE] Indication confirmed by client");
    }
}

static void notify_subscribed(struct bt_conn *conn, void *param)
{
    int rc;
    if (custom_ctx.conn && custom_ctx.notify_enabled) {
        rc = bt_gatt_notify(custom_ctx.conn, &custom_svc.attrs[1], &custom_ctx.value, sizeof(custom_ctx.value));
        if (rc == 0) {
            LOG_INF("[BLE] Notified: 0x%08X", custom_ctx.value);
        } else {
            LOG_ERR("[BLE] Notify error: %d", rc);
        }
    }
    if (custom_ctx.conn && custom_ctx.indicate_enabled) {
        ind_params.attr = &custom_svc.attrs[1];
        ind_params.func = indicate_cb;
        ind_params.data = &custom_ctx.value;
        ind_params.len = sizeof(custom_ctx.value);
        ind_params.destroy = NULL;
        rc = bt_gatt_indicate(custom_ctx.conn, &ind_params);
        if (rc == 0) {
            LOG_INF("[BLE] Indication sent: 0x%08X", custom_ctx.value);
        } else {
            LOG_ERR("[BLE] Indicate error: %d", rc);
        }
    }
}

// --- Shell and advertising logic ---
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static int advertise(void);

static void connected(struct bt_conn *conn, uint8_t hci_err)
{
    if (hci_err) {
        LOG_ERR("[BLE] Connected error: %d", hci_err);
        return;
    }
    custom_ctx.conn = conn;
    LOG_INF("[BLE] Connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ARG_UNUSED(conn);
    custom_ctx.conn = NULL;
    custom_ctx.notify_enabled = false;
    custom_ctx.indicate_enabled = false;
    LOG_INF("[BLE] Disconnected, reason: %d", reason);
}

static struct bt_conn_cb conn_cb = {
    .connected = connected,
    .disconnected = disconnected,
};

static int advertise(void)
{
    int rc;
    (void)bt_conn_cb_register(&conn_cb);
    rc = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (rc) {
        (void)bt_conn_cb_unregister(&conn_cb);
    }
    return rc;
}

static int cmd_adv(const struct shell *sh, size_t argc, char *argv[])
{
    bool start;
    int rc;
    if (!strcmp(argv[1], "on")) {
        start = true;
    } else if (!strcmp(argv[1], "off")) {
        start = false;
    } else {
        LOG_ERR("[BLE] Invalid argument: %s", argv[1]);
        return -EINVAL;
    }
    if (start) {
        rc = advertise();
    } else {
        rc = bt_le_adv_stop();
        (void)bt_conn_cb_unregister(&conn_cb);
    }
    if (rc < 0) {
        LOG_ERR("[BLE] Failed to %s advertising: %d", start ? "start" : "stop", rc);
        return -ENOEXEC;
    }
    LOG_INF("[BLE] Advertising %s", start ? "started" : "stopped");
    return 0;
}

static int cmd_read(const struct shell *sh, size_t argc, char *argv[])
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(sh, "Characteristic value: 0x%08X", custom_ctx.value);
    return 0;
}

static int cmd_write(const struct shell *sh, size_t argc, char *argv[])
{
    if (argc < 2) {
        LOG_ERR("[BLE] Usage: write <hex_value>");
        return -EINVAL;
    }
    uint32_t value;
    if (sscanf(argv[1], "%x", &value) != 1) {
        LOG_ERR("[BLE] Invalid hex value: %s", argv[1]);
        return -EINVAL;
    }
    custom_ctx.value = value;
    LOG_INF("[BLE] Wrote characteristic value: 0x%08X", value);
    notify_subscribed(custom_ctx.conn, NULL);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bt_test_cmds,
    SHELL_CMD_ARG(advertise, NULL, "on|off", cmd_adv, 2, 0),
    SHELL_CMD_ARG(read, NULL, "Read characteristic value", cmd_read, 1, 0),
    SHELL_CMD_ARG(write, NULL, "Write characteristic value <hex>", cmd_write, 2, 0),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(bt_test, &bt_test_cmds, "BLE test service commands", NULL, 2, 0);
