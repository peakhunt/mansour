#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

// --- Board Specific ---
#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU                OPT_MCU_RP2040
#endif

#ifndef CFG_TUSB_MCU
#define CFG_TUSB_OS                 OPT_OS_NONE      // No RTOS, pure Mansour Loop
#endif

// --- Device Stack ---
#define CFG_TUD_ENABLED             1
#define CFG_TUD_CDC                 1               // 1 CDC port for the Shell
#define CFG_TUD_MSC                 0
#define CFG_TUD_HID                 0
#define CFG_TUD_MIDI                0
#define CFG_TUD_VENDOR              0

// --- CDC FIFO ---
// Increase these if you plan to dump lots of data/logs at once
#define CFG_TUD_CDC_RX_BUFSIZE      256
#define CFG_TUD_CDC_TX_BUFSIZE      2048

// --- Endpoint Size ---
#define CFG_TUD_ENDPOINT0_SIZE      64

// RP2040 has 1 USB controller, we use it as Device (Port 0)
#define CFG_TUSB_RHPORT0_MODE       OPT_MODE_DEVICE

// Since we are on a single-port MCU, we can leave Port 1 as NONE
#define CFG_TUSB_RHPORT1_MODE       OPT_MODE_NONE

#endif
