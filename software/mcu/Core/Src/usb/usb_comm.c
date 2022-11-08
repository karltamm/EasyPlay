#include <USBd_cdc_if.h>
#include <string.h>

#include <USB_comm.h>

/* ABBREVIATIONS */
// TODO: Add list

/* GLOBAL VARIABLES */
extern uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];  // In a file USBd_cdc_if.c

static uint32_t g_USB_RX_msg_size = 0;
static Msg g_USB_RX_msgs[USB_RX_QUEUE_MAX_SIZE];
static Queue g_USB_RX_queue;

static Msg g_USB_TX_msgs[USB_TX_QUEUE_MAX_SIZE];
static Queue g_USB_TX_queue;

/* FUNCTIONS */
void notify_about_USB_RX_msg(uint32_t* msg_size) {
  g_USB_RX_msg_size = *msg_size;
}

void init_USB_queues() {
  init_queue(&g_USB_RX_queue, USB_RX_QUEUE_MAX_SIZE, g_USB_RX_msgs);
  init_queue(&g_USB_TX_queue, USB_TX_QUEUE_MAX_SIZE, g_USB_TX_msgs);
}

USB_QueueStatus add_USB_RX_msg_to_queue() {
  if (g_USB_RX_msg_size == 0) {
    return USB_QUEUE_FAIL;
  }

  Msg* msg_slot = get_queue_empty_slot(&g_USB_RX_queue);

  if (msg_slot == NULL || g_USB_RX_msg_size > MSG_MAX_SIZE) {
    g_USB_RX_msg_size = 0;
    return USB_QUEUE_FAIL;
  }

  memcpy(msg_slot->data, UserRxBufferFS, g_USB_RX_msg_size);
  g_USB_RX_msg_size = 0;

  return USB_QUEUE_OK;
}

USB_QueueStatus add_USB_TX_msg_to_queue(Msg* msg) {
  Msg* msg_slot = get_queue_empty_slot(&g_USB_TX_queue);
  if (msg_slot == NULL) {
    return USB_QUEUE_FAIL;
  }

  // Not checking input message length, because messages are hard coded.
  // It is assumed that those messages follow length limits.

  *msg_slot = *msg;

  return USB_QUEUE_OK;
}

void process_USB_RX_queue() {
  char* RX_msg;
  while (g_USB_RX_queue.cur_size > 0) {
    RX_msg = (char*)(get_msg_from_queue(&g_USB_RX_queue)->data);

    if (strcmp(RX_msg, USB_MSG_HANDSHAKE_IN) == 0) {
      CDC_Transmit_FS(USB_MSG_HANDSHAKE_OUT, USB_MSG_HANDSHAKE_OUT_SIZE);
    }

    // TODO: process LED commands
  }
}

void process_USB_TX_queue() {
  while (g_USB_TX_queue.cur_size > 0) {
    uint8_t* TX_msg = get_msg_from_queue(&g_USB_TX_queue)->data;
    uint16_t msg_size = strlen((const char*)TX_msg);
    while (CDC_Transmit_FS(TX_msg, msg_size) != USBD_OK) {
      // TODO: implement fail check to avoid infinite loop
    }
  }
}