#include "send_info/send_info.h"

#include "cmsis_os.h"
#include "imu/imu.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

#include <string.h>

#define USB_TX_BUF_SIZE 1024U
#define LOG_TIMEOUT_MS 5U

extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t tx_buf[USB_TX_BUF_SIZE];
static osSemaphoreId_t tx_free;

int usb_tx_init(void) {
    tx_free = osSemaphoreNew(1, 1, NULL); 

    if (tx_free == NULL) {
        return -1;
    }

    return 0;
}

int usb_send(const char *data, uint16_t len, uint32_t timeout_ms) {
    if (len == 0 || len > USB_TX_BUF_SIZE || tx_free == NULL) {
        return -1;
    }
    if (osSemaphoreAcquire(tx_free, timeout_ms) != osOK) {
        return -1;   // ocupado o puerto COM cerrado
    }

    memcpy(tx_buf, data, len);
    if (CDC_Transmit_FS(tx_buf, len) != USBD_OK) {
        osSemaphoreRelease(tx_free);
        return -1;
    }

    return 0;
}

void send_info(const char *message) {
    usb_send(message, (uint16_t)strlen(message), LOG_TIMEOUT_MS);
}

void usb_cdc_tx_complete(void) {
    if (tx_free != NULL) {
        osSemaphoreRelease(tx_free);
    }
}

void usb_cdc_reset(void) {
    if (tx_free != NULL) {
        osSemaphoreRelease(tx_free);
    }
}