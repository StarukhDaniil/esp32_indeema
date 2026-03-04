// #include "ble_manager.h"
// #include "host/ble_hs.h"
// #include "host/ble_uuid.h"
// #include "services/gap/ble_svc_gap.h"
// #include "services/gatt/ble_svc_gatt.h"
// #include "esp_random.h"

// #define GATT_BAS_UUID 0x180F
// #define GATT_CTS_UUID 0x1805
// #define GATT_DIS_UUID 0x180A

// int get_battery_level_cb() {
//     return esp_random() % 100;
// }

// static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
//     {
//         /* Service: Battery Service */
//         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//         .uuid = BLE_UUID16_DECLARE(GATT_BAS_UUID),
//         .characteristics = (struct ble_gatt_chr_def[])
//         { {
//                 /* Characteristic: get battery level */
//                 .uuid = BLE_UUID16_DECLARE(GATT_HRS_MEASUREMENT_UUID),
//                 .access_cb = get_battery_level_cb,
//                 .val_handle = &hrs_hrm_handle,
//                 .flags = BLE_GATT_CHR_F_NOTIFY,
//             }, {
//                 0, /* No more characteristics in this service */
//             },
//         }
//     },

//     {
//         /* Service: Heart-rate */
//         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//         .uuid = BLE_UUID16_DECLARE(GATT_HRS_UUID),
//         .characteristics = (struct ble_gatt_chr_def[])
//         { {
//                 /* Characteristic: Heart-rate measurement */
//                 .uuid = BLE_UUID16_DECLARE(GATT_HRS_MEASUREMENT_UUID),
//                 .access_cb = gatt_svr_chr_access_heart_rate,
//                 .val_handle = &hrs_hrm_handle,
//                 .flags = BLE_GATT_CHR_F_NOTIFY,
//             }, {
//                 /* Characteristic: Body sensor location */
//                 .uuid = BLE_UUID16_DECLARE(GATT_HRS_BODY_SENSOR_LOC_UUID),
//                 .access_cb = gatt_svr_chr_access_heart_rate,
//                 .flags = BLE_GATT_CHR_F_READ,
//             }, {
//                 0, /* No more characteristics in this service */
//             },
//         }
//     },

//     {
//         /* Service: Device Information */
//         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//         .uuid = BLE_UUID16_DECLARE(GATT_DEVICE_INFO_UUID),
//         .characteristics = (struct ble_gatt_chr_def[])
//         { {
//                 /* Characteristic: * Manufacturer name */
//                 .uuid = BLE_UUID16_DECLARE(GATT_MANUFACTURER_NAME_UUID),
//                 .access_cb = gatt_svr_chr_access_device_info,
//                 .flags = BLE_GATT_CHR_F_READ,
//             }, {
//                 /* Characteristic: Model number string */
//                 .uuid = BLE_UUID16_DECLARE(GATT_MODEL_NUMBER_UUID),
//                 .access_cb = gatt_svr_chr_access_device_info,
//                 .flags = BLE_GATT_CHR_F_READ,
//             }, {
//                 0, /* No more characteristics in this service */
//             },
//         }
//     },

//     {
//         0, /* No more services */
//     },
// };

// void blem_start_ble(blem_handle_t* blem) {

// }