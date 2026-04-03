#include "tusb.h"
#include "shell.h"
#include "circ_buffer.h"
#include "shell_if_usb.h"

#define USB_TX_SOFT_BUF_SIZE 1024
static uint8_t _usb_tx_raw_buf[USB_TX_SOFT_BUF_SIZE];
static CircBuffer _usb_tx_cb;

static ShellIntf              _shell_usb_if;

static bool
shell_if_usb_get_rx_data(ShellIntf* intf, uint8_t* data)
{
  if (tud_cdc_available())
  {
    uint32_t count = tud_cdc_read(data, 1);
    return (count > 0);
  }
  return false;
}

#ifdef SHELL_IF_USB_USE_CIRC_BUFFER
static bool
shell_if_usb_put_tx_data(ShellIntf* intf, uint8_t* data, uint16_t len)
{
  if (!tud_cdc_connected()) return true;

  uint32_t avail = tud_cdc_write_available();

  // to maintain tx sequence
  if(_usb_tx_cb.num_bytes != 0)
  {
    circ_buffer_enqueue(&_usb_tx_cb, data, len, 0);
  }
  else if(avail < len)
  {
    uint32_t remaining = len - avail;

    circ_buffer_enqueue(&_usb_tx_cb, &data[avail], remaining, 0);

    tud_cdc_write(data, avail);
    tud_cdc_write_flush();
  }
  else
  {
    tud_cdc_write(data, len);
    tud_cdc_write_flush();
  }
  return true;
}

void
tud_cdc_tx_complete_cb(uint8_t itf)
{
  //
  // XXX
  // called only when CDC TX buffer is empty
  //
  if (itf != 0) return;

  uint32_t avail = tud_cdc_write_available();

  while(_usb_tx_cb.num_bytes != 0 && avail != 0)
  {
    uint8_t tx_buf[64]; // Standard USB packet size
    uint32_t tx_len = _usb_tx_cb.num_bytes;

    if(tx_len > avail) tx_len = avail;
    if(tx_len > 64) tx_len = 64;

    circ_buffer_dequeue(&_usb_tx_cb, tx_buf, tx_len, 0);
    tud_cdc_write(tx_buf, tx_len);
    tud_cdc_write_flush();
    avail = tud_cdc_write_available();
  }
}
#else
static bool
shell_if_usb_put_tx_data(ShellIntf* intf, uint8_t* data, uint16_t len)
{
  if (!tud_cdc_connected()) return true;

  uint32_t avail = tud_cdc_write_available();
  
  uint32_t write_len = (len < avail) ? len : avail;
  
  // XXX
  // what about remaining data? we don't fucking care
  // 
  tud_cdc_write(data, write_len);
  tud_cdc_write_flush();
  
  return true;
}

void
tud_cdc_tx_complete_cb(uint8_t itf)
{
}
#endif

void
tud_cdc_rx_cb(uint8_t itf)
{
  uint8_t b;

  // Keep reading until the TinyUSB FIFO is totally empty
  while (tud_cdc_available() > 0)
  {
    shell_handle_rx(&_shell_usb_if);
  }
}


void
shell_if_usb_init(void)
{
  _shell_usb_if.cmd_buffer_ndx    = 0;
  _shell_usb_if.get_rx_data       = shell_if_usb_get_rx_data;
  _shell_usb_if.put_tx_data       = shell_if_usb_put_tx_data;

  INIT_LIST_HEAD(&_shell_usb_if.lh);

  circ_buffer_init(&_usb_tx_cb, _usb_tx_raw_buf, USB_TX_SOFT_BUF_SIZE, NULL, NULL);
  shell_if_register(&_shell_usb_if);
}
