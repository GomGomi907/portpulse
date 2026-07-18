#include "ch552_regs.h"
#include "usb_cdc.h"
#include "usb_descriptors.h"

#define USB_EP0_SIZE 8u
#define USB_EP2_SIZE 64u

#define USB_REQ_GET_STATUS 0x00u
#define USB_REQ_CLEAR_FEATURE 0x01u
#define USB_REQ_SET_FEATURE 0x03u
#define USB_REQ_SET_ADDRESS 0x05u
#define USB_REQ_GET_DESCRIPTOR 0x06u
#define USB_REQ_GET_CONFIGURATION 0x08u
#define USB_REQ_SET_CONFIGURATION 0x09u
#define USB_REQ_GET_INTERFACE 0x0Au
#define USB_REQ_SET_INTERFACE 0x0Bu

#define CDC_REQ_SET_LINE_CODING 0x20u
#define CDC_REQ_GET_LINE_CODING 0x21u
#define CDC_REQ_SET_CONTROL_LINE_STATE 0x22u
#define CDC_REQ_SEND_BREAK 0x23u

#define CONTROL_IDLE 0u
#define CONTROL_IN_DATA 1u
#define CONTROL_STATUS_IN 2u
#define CONTROL_LINE_CODING_OUT 3u
#define USB_FEATURE_ENDPOINT_HALT 0u
#define ENDPOINT_HALT_EP1_IN 0x01u
#define ENDPOINT_HALT_EP2_OUT 0x02u
#define ENDPOINT_HALT_EP2_IN 0x04u
#define USB_CDC_TX_SPIN_LIMIT 60000u

#ifdef USB_CDC_HOST_TEST
#define USB_XDATA
#define USB_AT(address)
#define USB_DMA_ADDRESS(buffer) 0u
#define USB_CLEAR_FLAG(bit_name, mask) \
    do { \
        (void)(bit_name); \
        USB_INT_FG &= (uint8_t)~(mask); \
    } while (0)
#else
#define USB_XDATA __xdata
#define USB_AT(address) __at(address)
#define USB_DMA_ADDRESS(buffer) ((uint16_t)(buffer))
#define USB_CLEAR_FLAG(bit_name, mask) \
    do { \
        (void)(mask); \
        (bit_name) = 0; \
    } while (0)
#endif

/*
 * CH552 reserves hardware-sized DMA regions independently of the endpoint
 * descriptor packet size: EP0 needs 64 bytes, EP1 TX needs 64 bytes, and
 * EP2's single RX/TX mode needs 128 bytes.
 */
USB_XDATA USB_AT(0x0000) uint8_t usb_ep0_buffer[64];
USB_XDATA USB_AT(0x0040) uint8_t usb_ep1_buffer[64];
USB_XDATA USB_AT(0x0080) uint8_t usb_ep2_buffer[USB_EP2_SIZE * 2u];

static USB_XDATA uint8_t serial_descriptor[USB_SERIAL_DESCRIPTOR_LENGTH];
static USB_XDATA uint8_t control_response[8];
static USB_XDATA uint8_t line_coding[7];

static const uint8_t *control_pointer;
static uint16_t control_remaining;
static uint8_t control_state;
static uint8_t control_send_zlp;
static uint8_t pending_address;
static uint8_t address_pending;
static volatile uint8_t current_configuration;
static volatile uint8_t ep2_out_length;
static volatile uint8_t ep2_out_index;
static volatile uint8_t ep2_tx_busy;
static volatile uint8_t endpoint_halt;

static uint16_t setup_u16(uint8_t offset)
{
    return (uint16_t)usb_ep0_buffer[offset] |
        ((uint16_t)usb_ep0_buffer[(uint8_t)(offset + 1u)] << 8);
}

static void endpoint0_stall(void)
{
    UEP0_T_LEN = 0u;
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG |
        UEP_R_RES_STALL | UEP_T_RES_STALL;
    control_state = CONTROL_IDLE;
}

static void endpoint0_status_in(void)
{
    control_state = CONTROL_STATUS_IN;
    UEP0_T_LEN = 0u;
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG |
        UEP_R_RES_ACK | UEP_T_RES_ACK;
}

static void endpoint0_queue_next(void)
{
    uint8_t count;
    uint8_t index;

    if (control_remaining != 0u) {
        count = (uint8_t)(control_remaining > USB_EP0_SIZE ?
            USB_EP0_SIZE : control_remaining);
        for (index = 0u; index < count; ++index) {
            usb_ep0_buffer[index] = *control_pointer++;
        }
        control_remaining = (uint16_t)(control_remaining - count);
        UEP0_T_LEN = count;
        UEP0_CTRL = (UEP0_CTRL & (uint8_t)~MASK_UEP_T_RES) | UEP_T_RES_ACK;
        return;
    }

    if (control_send_zlp) {
        control_send_zlp = 0u;
        UEP0_T_LEN = 0u;
        UEP0_CTRL = (UEP0_CTRL & (uint8_t)~MASK_UEP_T_RES) | UEP_T_RES_ACK;
        return;
    }

    UEP0_T_LEN = 0u;
    UEP0_CTRL = (UEP0_CTRL & (uint8_t)~MASK_UEP_T_RES) | UEP_T_RES_NAK;
    control_state = CONTROL_IDLE;
}

static void endpoint0_start_in(
    const uint8_t *data,
    uint16_t available,
    uint16_t requested
)
{
    uint16_t send_length;

    send_length = available;
    if (send_length > requested) {
        send_length = requested;
    }
    control_pointer = data;
    control_remaining = send_length;
    control_send_zlp = (uint8_t)(
        (send_length < requested) &&
        ((send_length & (USB_EP0_SIZE - 1u)) == 0u)
    );
    control_state = CONTROL_IN_DATA;
    endpoint0_queue_next();
}

static void reset_data_endpoints(void)
{
    ep2_out_length = 0u;
    ep2_out_index = 0u;
    ep2_tx_busy = 0u;
    endpoint_halt = 0u;
    UEP1_T_LEN = 0u;
    UEP2_T_LEN = 0u;
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
    UEP2_CTRL = bUEP_AUTO_TOG |
        ((current_configuration != 0u) ? UEP_R_RES_ACK : UEP_R_RES_NAK) |
        UEP_T_RES_NAK;
}

static void handle_get_descriptor(
    uint16_t value,
    uint16_t language_id,
    uint16_t requested
)
{
    const uint8_t *descriptor;
    uint16_t length;
    uint8_t descriptor_type;
    uint8_t descriptor_index;

    descriptor_type = (uint8_t)(value >> 8);
    descriptor_index = (uint8_t)value;
    if (((descriptor_type == USB_DESCRIPTOR_DEVICE) ||
         (descriptor_type == USB_DESCRIPTOR_CONFIGURATION)) &&
        ((descriptor_index != 0u) || (language_id != 0u))) {
        endpoint0_stall();
        return;
    }
    if (descriptor_type == USB_DESCRIPTOR_STRING) {
        if (((descriptor_index == 0u) && (language_id != 0u)) ||
            ((descriptor_index != 0u) && (language_id != 0x0409u))) {
            endpoint0_stall();
            return;
        }
    }
    if ((descriptor_type == USB_DESCRIPTOR_STRING) &&
        (descriptor_index == 3u)) {
        endpoint0_start_in(serial_descriptor, USB_SERIAL_DESCRIPTOR_LENGTH, requested);
        return;
    }

    descriptor = usb_descriptor_lookup(descriptor_type, descriptor_index, &length);
    if (descriptor == 0) {
        endpoint0_stall();
        return;
    }
    endpoint0_start_in(descriptor, length, requested);
}

static void handle_standard_setup(
    uint8_t request_type,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint16_t length
)
{
    uint8_t halt_mask;

    halt_mask = 0u;

    switch (request) {
    case USB_REQ_GET_DESCRIPTOR:
        if (request_type != 0x80u) {
            endpoint0_stall();
        } else {
            handle_get_descriptor(value, index, length);
        }
        break;
    case USB_REQ_SET_ADDRESS:
        if ((request_type != 0x00u) || (value > 127u) ||
            (index != 0u) || (length != 0u)) {
            endpoint0_stall();
        } else {
            pending_address = (uint8_t)value;
            address_pending = 1u;
            endpoint0_status_in();
        }
        break;
    case USB_REQ_SET_CONFIGURATION:
        if ((request_type != 0x00u) || (value > 1u) ||
            (index != 0u) || (length != 0u)) {
            endpoint0_stall();
        } else {
            current_configuration = (uint8_t)value;
            reset_data_endpoints();
            endpoint0_status_in();
        }
        break;
    case USB_REQ_GET_CONFIGURATION:
        if ((request_type != 0x80u) || (value != 0u) ||
            (index != 0u) || (length != 1u)) {
            endpoint0_stall();
        } else {
            control_response[0] = current_configuration;
            endpoint0_start_in(control_response, 1u, length);
        }
        break;
    case USB_REQ_GET_STATUS:
        if ((value != 0u) || (length != 2u)) {
            endpoint0_stall();
            break;
        }
        if ((request_type == 0x80u) && (index == 0u)) {
            control_response[0] = 0u;
            control_response[1] = 0u;
            endpoint0_start_in(control_response, 2u, length);
        } else if ((request_type == 0x81u) && (index <= 1u)) {
            control_response[0] = 0u;
            control_response[1] = 0u;
            endpoint0_start_in(control_response, 2u, length);
        } else if (request_type == 0x82u) {
            if ((index == 0x00u) || (index == 0x80u)) {
                halt_mask = 0u;
            } else if (index == 0x81u) {
                halt_mask = ENDPOINT_HALT_EP1_IN;
            } else if (index == 0x02u) {
                halt_mask = ENDPOINT_HALT_EP2_OUT;
            } else if (index == 0x82u) {
                halt_mask = ENDPOINT_HALT_EP2_IN;
            } else {
                endpoint0_stall();
                break;
            }
            control_response[0] =
                (halt_mask != 0u) && (endpoint_halt & halt_mask) ? 1u : 0u;
            control_response[1] = 0u;
            endpoint0_start_in(control_response, 2u, length);
        } else {
            endpoint0_stall();
        }
        break;
    case USB_REQ_GET_INTERFACE:
        if ((request_type != 0x81u) || (value != 0u) ||
            (index > 1u) || (length != 1u)) {
            endpoint0_stall();
        } else {
            control_response[0] = 0u;
            endpoint0_start_in(control_response, 1u, length);
        }
        break;
    case USB_REQ_SET_INTERFACE:
        if ((request_type != 0x01u) || (value != 0u) ||
            (index > 1u) || (length != 0u)) {
            endpoint0_stall();
        } else {
            endpoint0_status_in();
        }
        break;
    case USB_REQ_CLEAR_FEATURE:
        if ((request_type != 0x02u) ||
            (value != USB_FEATURE_ENDPOINT_HALT) ||
            (length != 0u)) {
            endpoint0_stall();
            break;
        }
        if (index == 0x81u) {
            endpoint_halt &= (uint8_t)~ENDPOINT_HALT_EP1_IN;
            UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
        } else if (index == 0x02u) {
            endpoint_halt &= (uint8_t)~ENDPOINT_HALT_EP2_OUT;
            ep2_out_length = 0u;
            ep2_out_index = 0u;
            UEP2_CTRL = (UEP2_CTRL &
                (uint8_t)~(bUEP_R_TOG | MASK_UEP_R_RES)) |
                UEP_R_RES_ACK;
        } else if (index == 0x82u) {
            endpoint_halt &= (uint8_t)~ENDPOINT_HALT_EP2_IN;
            ep2_tx_busy = 0u;
            UEP2_T_LEN = 0u;
            UEP2_CTRL = (UEP2_CTRL &
                (uint8_t)~(bUEP_T_TOG | MASK_UEP_T_RES)) |
                UEP_T_RES_NAK;
        } else {
            endpoint0_stall();
            break;
        }
        endpoint0_status_in();
        break;
    case USB_REQ_SET_FEATURE:
        if ((request_type != 0x02u) ||
            (value != USB_FEATURE_ENDPOINT_HALT) ||
            (length != 0u)) {
            endpoint0_stall();
            break;
        }
        if (index == 0x81u) {
            endpoint_halt |= ENDPOINT_HALT_EP1_IN;
            UEP1_CTRL = (UEP1_CTRL & (uint8_t)~MASK_UEP_T_RES) |
                UEP_T_RES_STALL;
        } else if (index == 0x02u) {
            endpoint_halt |= ENDPOINT_HALT_EP2_OUT;
            UEP2_CTRL = (UEP2_CTRL & (uint8_t)~MASK_UEP_R_RES) |
                UEP_R_RES_STALL;
        } else if (index == 0x82u) {
            endpoint_halt |= ENDPOINT_HALT_EP2_IN;
            ep2_tx_busy = 0u;
            UEP2_T_LEN = 0u;
            UEP2_CTRL = (UEP2_CTRL & (uint8_t)~MASK_UEP_T_RES) |
                UEP_T_RES_STALL;
        } else {
            endpoint0_stall();
            break;
        }
        endpoint0_status_in();
        break;
    default:
        endpoint0_stall();
        break;
    }
}

static void handle_cdc_setup(
    uint8_t request_type,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint16_t length
)
{
    if (index != 0u) {
        endpoint0_stall();
        return;
    }

    switch (request) {
    case CDC_REQ_SET_LINE_CODING:
        if ((request_type != 0x21u) || (value != 0u) || (length != 7u)) {
            endpoint0_stall();
        } else {
            control_state = CONTROL_LINE_CODING_OUT;
            UEP0_T_LEN = 0u;
            UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG |
                UEP_R_RES_ACK | UEP_T_RES_NAK;
        }
        break;
    case CDC_REQ_GET_LINE_CODING:
        if ((request_type != 0xA1u) || (value != 0u) || (length != 7u)) {
            endpoint0_stall();
        } else {
            endpoint0_start_in(line_coding, sizeof(line_coding), length);
        }
        break;
    case CDC_REQ_SET_CONTROL_LINE_STATE:
    case CDC_REQ_SEND_BREAK:
        if ((request_type != 0x21u) || (length != 0u)) {
            endpoint0_stall();
        } else {
            endpoint0_status_in();
        }
        break;
    default:
        endpoint0_stall();
        break;
    }
}

static void handle_setup(void)
{
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;

    request_type = usb_ep0_buffer[0];
    request = usb_ep0_buffer[1];
    value = setup_u16(2u);
    index = setup_u16(4u);
    length = setup_u16(6u);

    control_state = CONTROL_IDLE;
    control_remaining = 0u;
    control_send_zlp = 0u;
    address_pending = 0u;
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG |
        UEP_R_RES_ACK | UEP_T_RES_NAK;

    if ((request_type & 0x60u) == 0x00u) {
        handle_standard_setup(request_type, request, value, index, length);
    } else if ((request_type & 0x60u) == 0x20u) {
        handle_cdc_setup(request_type, request, value, index, length);
    } else {
        endpoint0_stall();
    }
}

static void handle_endpoint0_in(void)
{
    if (control_state == CONTROL_STATUS_IN) {
        UEP0_T_LEN = 0u;
        UEP0_CTRL = (UEP0_CTRL & (uint8_t)~MASK_UEP_T_RES) | UEP_T_RES_NAK;
        control_state = CONTROL_IDLE;
        if (address_pending) {
            USB_DEV_AD = pending_address;
            address_pending = 0u;
        }
        return;
    }

    if (control_state == CONTROL_IN_DATA) {
        UEP0_CTRL ^= bUEP_T_TOG;
        endpoint0_queue_next();
    } else {
        UEP0_T_LEN = 0u;
        UEP0_CTRL = (UEP0_CTRL & (uint8_t)~MASK_UEP_T_RES) | UEP_T_RES_NAK;
    }
}

static void handle_endpoint0_out(void)
{
    uint8_t index;

    if (control_state == CONTROL_LINE_CODING_OUT) {
        if (USB_RX_LEN != sizeof(line_coding)) {
            endpoint0_stall();
            return;
        }
        for (index = 0u; index < sizeof(line_coding); ++index) {
            line_coding[index] = usb_ep0_buffer[index];
        }
        endpoint0_status_in();
        return;
    }

    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG |
        UEP_R_RES_ACK | UEP_T_RES_NAK;
    control_state = CONTROL_IDLE;
}

static void reset_usb_state(void)
{
    current_configuration = 0u;
    address_pending = 0u;
    control_state = CONTROL_IDLE;
    USB_DEV_AD = 0u;
    UEP0_T_LEN = 0u;
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    reset_data_endpoints();
}

void usb_cdc_init(const uint8_t chip_id[5])
{
    uint16_t address;

    usb_build_serial_descriptor(serial_descriptor, chip_id);

    line_coding[0] = 0x00u;
    line_coding[1] = 0xC2u;
    line_coding[2] = 0x01u;
    line_coding[3] = 0x00u;
    line_coding[4] = 0u;
    line_coding[5] = 0u;
    line_coding[6] = 8u;

    address = USB_DMA_ADDRESS(usb_ep0_buffer);
    UEP0_DMA_L = (uint8_t)address;
    UEP0_DMA_H = (uint8_t)(address >> 8);
    address = USB_DMA_ADDRESS(usb_ep1_buffer);
    UEP1_DMA_L = (uint8_t)address;
    UEP1_DMA_H = (uint8_t)(address >> 8);
    address = USB_DMA_ADDRESS(usb_ep2_buffer);
    UEP2_DMA_L = (uint8_t)address;
    UEP2_DMA_H = (uint8_t)(address >> 8);

    UEP4_1_MOD = bUEP1_TX_EN;
    UEP2_3_MOD = bUEP2_RX_EN | bUEP2_TX_EN;
    reset_usb_state();

#ifdef USB_CDC_HOST_TEST
    USB_INT_FG = 0u;
#else
    USB_INT_FG = 0xFFu;
#endif
    USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
    USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;
    UDEV_CTRL = bUD_PD_DIS | bUD_PORT_EN;
    IE_USB = 1;
    EA = 1;
}

void usb_cdc_isr(void)
{
    uint8_t status;
    uint8_t endpoint;
    uint8_t token;

    if (USB_INT_FG & UIF_TRANSFER) {
        status = USB_INT_ST;
        endpoint = status & MASK_UIS_ENDP;
        token = status & MASK_UIS_TOKEN;

        if ((endpoint == 0u) && (token == UIS_TOKEN_SETUP)) {
            handle_setup();
        } else if ((endpoint == 0u) && (token == UIS_TOKEN_IN)) {
            handle_endpoint0_in();
        } else if ((endpoint == 0u) && (token == UIS_TOKEN_OUT)) {
            if (status & bUIS_TOG_OK) {
                handle_endpoint0_out();
            }
        } else if ((endpoint == 2u) && (token == UIS_TOKEN_OUT)) {
            if (current_configuration == 0u) {
                ep2_out_length = 0u;
                ep2_out_index = 0u;
                UEP2_CTRL = (UEP2_CTRL & (uint8_t)~MASK_UEP_R_RES) |
                    UEP_R_RES_NAK;
            } else if (status & bUIS_TOG_OK) {
                ep2_out_length = USB_RX_LEN;
                ep2_out_index = 0u;
                if (endpoint_halt & ENDPOINT_HALT_EP2_OUT) {
                    UEP2_CTRL = (UEP2_CTRL & (uint8_t)~MASK_UEP_R_RES) |
                        UEP_R_RES_STALL;
                } else if (ep2_out_length == 0u) {
                    UEP2_CTRL = (UEP2_CTRL & (uint8_t)~MASK_UEP_R_RES) |
                        UEP_R_RES_ACK;
                } else {
                    UEP2_CTRL = (UEP2_CTRL & (uint8_t)~MASK_UEP_R_RES) |
                        UEP_R_RES_NAK;
                }
            }
        } else if ((endpoint == 2u) && (token == UIS_TOKEN_IN)) {
            UEP2_T_LEN = 0u;
            ep2_tx_busy = 0u;
            UEP2_CTRL = (UEP2_CTRL & (uint8_t)~MASK_UEP_T_RES) |
                ((endpoint_halt & ENDPOINT_HALT_EP2_IN) ?
                 UEP_T_RES_STALL : UEP_T_RES_NAK);
        }
        USB_CLEAR_FLAG(UIF_TRANSFER_BIT, UIF_TRANSFER);
    }

    if (USB_INT_FG & UIF_BUS_RST) {
        reset_usb_state();
        USB_CLEAR_FLAG(UIF_BUS_RST_BIT, UIF_BUS_RST);
    }
    if (USB_INT_FG & UIF_SUSPEND) {
        ep2_tx_busy = 0u;
        UEP2_T_LEN = 0u;
        UEP2_CTRL = (UEP2_CTRL & (uint8_t)~MASK_UEP_T_RES) |
            ((endpoint_halt & ENDPOINT_HALT_EP2_IN) ?
             UEP_T_RES_STALL : UEP_T_RES_NAK);
        USB_CLEAR_FLAG(UIF_SUSPEND_BIT, UIF_SUSPEND);
    }
    if (USB_INT_FG & UIF_FIFO_OV) {
        USB_CLEAR_FLAG(UIF_FIFO_OV_BIT, UIF_FIFO_OV);
    }
}

uint8_t usb_cdc_configured(void)
{
    return (uint8_t)(current_configuration != 0u);
}

uint8_t usb_cdc_available(void)
{
    if (current_configuration == 0u) {
        return 0u;
    }
    return (uint8_t)(ep2_out_length - ep2_out_index);
}

uint8_t usb_cdc_read(void)
{
    uint8_t value;

    if ((current_configuration == 0u) ||
        (ep2_out_index >= ep2_out_length)) {
        return 0u;
    }

    value = usb_ep2_buffer[ep2_out_index++];
    if (ep2_out_index == ep2_out_length) {
        ep2_out_index = 0u;
        ep2_out_length = 0u;
        UEP2_CTRL = (UEP2_CTRL & (uint8_t)~MASK_UEP_R_RES) |
            ((current_configuration == 0u) ? UEP_R_RES_NAK :
             (endpoint_halt & ENDPOINT_HALT_EP2_OUT) ?
             UEP_R_RES_STALL : UEP_R_RES_ACK);
    }
    return value;
}

static uint8_t wait_for_tx_ready(void)
{
    uint16_t spins;

    spins = USB_CDC_TX_SPIN_LIMIT;
    while (ep2_tx_busy) {
        if ((current_configuration == 0u) ||
            (endpoint_halt & ENDPOINT_HALT_EP2_IN) ||
            (--spins == 0u)) {
            ep2_tx_busy = 0u;
            UEP2_T_LEN = 0u;
            UEP2_CTRL = (UEP2_CTRL & (uint8_t)~MASK_UEP_T_RES) |
                ((endpoint_halt & ENDPOINT_HALT_EP2_IN) ?
                 UEP_T_RES_STALL : UEP_T_RES_NAK);
            return 0u;
        }
    }
    return 1u;
}

uint8_t usb_cdc_write(const uint8_t *data, uint8_t length)
{
    uint8_t chunk;
    uint8_t index;

    if ((current_configuration == 0u) ||
        (endpoint_halt & ENDPOINT_HALT_EP2_IN)) {
        return 0u;
    }
    while (length != 0u) {
        if (!wait_for_tx_ready()) {
            return 0u;
        }
        chunk = length > USB_EP2_SIZE ? USB_EP2_SIZE : length;
        for (index = 0u; index < chunk; ++index) {
            usb_ep2_buffer[USB_EP2_SIZE + index] = *data++;
        }
        ep2_tx_busy = 1u;
        UEP2_T_LEN = chunk;
        UEP2_CTRL = (UEP2_CTRL & (uint8_t)~MASK_UEP_T_RES) |
            UEP_T_RES_ACK;
        length = (uint8_t)(length - chunk);
    }
    return 1u;
}

uint8_t usb_cdc_flush(void)
{
    return wait_for_tx_ready();
}
