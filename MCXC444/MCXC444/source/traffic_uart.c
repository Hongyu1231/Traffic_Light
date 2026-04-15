#include "traffic_uart.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app_context.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

static char send_buffer[MAX_MSG_LEN];
static const char UART_KEY[] = "CG2271_UART_KEY";

static char toHexNibble(uint8_t value)
{
    value &= 0x0FU;
    return (value < 10U) ? (char)('0' + value) : (char)('A' + value - 10);
}

static int fromHexNibble(char value)
{
    if ((value >= '0') && (value <= '9')) {
        return value - '0';
    }
    if ((value >= 'A') && (value <= 'F')) {
        return value - 'A' + 10;
    }
    if ((value >= 'a') && (value <= 'f')) {
        return value - 'a' + 10;
    }
    return -1;
}

static bool makeSecurePacket(const char *plain_text, char *out_packet, size_t out_size)
{
    size_t plain_len = strlen(plain_text);
    size_t key_len = strlen(UART_KEY);
    uint8_t checksum = 0U;
    size_t out_index = 0U;

    if (out_size < ((plain_len * 2U) + 9U)) {
        return false;
    }

    out_packet[out_index++] = 'S';
    out_packet[out_index++] = 'E';
    out_packet[out_index++] = 'C';
    out_packet[out_index++] = ':';

    for (size_t i = 0U; i < plain_len; i++) {
        uint8_t encrypted = ((uint8_t)plain_text[i]) ^ ((uint8_t)UART_KEY[i % key_len]);
        checksum ^= encrypted;
        out_packet[out_index++] = toHexNibble(encrypted >> 4U);
        out_packet[out_index++] = toHexNibble(encrypted);
    }

    out_packet[out_index++] = ':';
    out_packet[out_index++] = toHexNibble(checksum >> 4U);
    out_packet[out_index++] = toHexNibble(checksum);
    out_packet[out_index++] = '\n';
    out_packet[out_index] = '\0';

    return true;
}

static bool decodeSecurePacket(const char *packet, char *plain_text, size_t plain_size)
{
    char clean_packet[MAX_MSG_LEN];
    size_t packet_len;
    size_t key_len = strlen(UART_KEY);
    const char *checksum_separator = NULL;
    uint8_t expected_checksum;
    uint8_t actual_checksum = 0U;
    size_t cipher_len;
    size_t plain_index = 0U;
    int hi;
    int lo;

    strncpy(clean_packet, packet, sizeof(clean_packet));
    clean_packet[sizeof(clean_packet) - 1U] = '\0';
    clean_packet[strcspn(clean_packet, "\r\n")] = '\0';

    packet = clean_packet;
    packet_len = strlen(packet);

    if ((packet_len < 8U) || (strncmp(packet, "SEC:", 4U) != 0)) {
        PRINTF("Secure decode failed: missing SEC prefix\r\n");
        return false;
    }

    checksum_separator = strrchr(packet, ':');
    if ((checksum_separator == NULL) || (checksum_separator <= (packet + 4)) ||
        ((size_t)(checksum_separator - packet + 2U) >= packet_len)) {
        PRINTF("Secure decode failed: bad checksum separator\r\n");
        return false;
    }

    hi = fromHexNibble(checksum_separator[1]);
    lo = fromHexNibble(checksum_separator[2]);
    if ((hi < 0) || (lo < 0)) {
        PRINTF("Secure decode failed: checksum is not hex\r\n");
        return false;
    }
    expected_checksum = (uint8_t)((hi << 4) | lo);

    cipher_len = (size_t)(checksum_separator - (packet + 4));
    if ((cipher_len % 2U) != 0U) {
        PRINTF("Secure decode failed: odd encrypted length\r\n");
        return false;
    }

    if (((cipher_len / 2U) + 1U) > plain_size) {
        PRINTF("Secure decode failed: plaintext buffer too small\r\n");
        return false;
    }

    for (size_t i = 0U; i < cipher_len; i += 2U) {
        int byte_hi = fromHexNibble(packet[4U + i]);
        int byte_lo = fromHexNibble(packet[4U + i + 1U]);
        uint8_t encrypted;

        if ((byte_hi < 0) || (byte_lo < 0)) {
            PRINTF("Secure decode failed: encrypted payload is not hex\r\n");
            return false;
        }

        encrypted = (uint8_t)((byte_hi << 4) | byte_lo);
        actual_checksum ^= encrypted;
        plain_text[plain_index] = (char)(encrypted ^ ((uint8_t)UART_KEY[plain_index % key_len]));
        plain_index++;
    }

    plain_text[plain_index] = '\0';

    if (actual_checksum != expected_checksum) {
        PRINTF("Secure decode failed: checksum mismatch expected=%02X actual=%02X\r\n",
               expected_checksum,
               actual_checksum);
        return false;
    }

    return true;
}

void sendMessage(const char *message)
{
    strncpy(send_buffer, message, MAX_MSG_LEN);
    send_buffer[MAX_MSG_LEN - 1U] = '\0';

    UART2->C2 |= UART_C2_TIE_MASK;
    UART2->C2 |= UART_C2_TE_MASK;
}

void sendSpeedBand(uint8_t band)
{
    char plain_buffer[MAX_MSG_LEN];
    char secure_buffer[MAX_MSG_LEN];

    if ((band == 0U) || (band > 9U)) {
        return;
    }

    snprintf(plain_buffer, sizeof(plain_buffer), "MCX:%u", (unsigned int)band);

    if (makeSecurePacket(plain_buffer, secure_buffer, sizeof(secure_buffer))) {
        sendMessage(secure_buffer);
        PRINTF("Sent secure speed band to ESP32: %s\r\n", plain_buffer);
    } else {
        PRINTF("Failed to build secure UART speed packet\r\n");
    }
}

void initUART2(uint32_t baud_rate)
{
    uint32_t bus_clk;
    uint32_t sbr;

    NVIC_DisableIRQ(UART2_FLEXIO_IRQn);

    SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    UART2->C2 &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);

    PORTE->PCR[UART_TX_PTE22] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[UART_TX_PTE22] |= PORT_PCR_MUX(4);

    PORTE->PCR[UART_RX_PTE23] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[UART_RX_PTE23] |= PORT_PCR_MUX(4);

    bus_clk = CLOCK_GetBusClkFreq();
    sbr = (bus_clk + (baud_rate * 8U)) / (baud_rate * 16U);

    UART2->BDH &= ~UART_BDH_SBR_MASK;
    UART2->BDH |= (uint8_t)((sbr >> 8U) & UART_BDH_SBR_MASK);
    UART2->BDL = (uint8_t)(sbr & 0xFFU);

    UART2->C1 &= ~(UART_C1_LOOPS_MASK | UART_C1_RSRC_MASK | UART_C1_PE_MASK | UART_C1_M_MASK);

    UART2->C2 |= UART_C2_RIE_MASK;
    UART2->C2 |= UART_C2_RE_MASK;

    NVIC_SetPriority(UART2_FLEXIO_IRQn, UART2_INT_PRIO);
    NVIC_ClearPendingIRQ(UART2_FLEXIO_IRQn);
    NVIC_EnableIRQ(UART2_FLEXIO_IRQn);
}

void UART2_FLEXIO_IRQHandler(void)
{
    static int recv_ptr = 0;
    static int send_ptr = 0;
    static char recv_buffer[MAX_MSG_LEN];
    BaseType_t hpw = pdFALSE;

    if ((UART2->S1 & UART_S1_TDRE_MASK) != 0U) {
        if (send_buffer[send_ptr] == '\0') {
            send_ptr = 0;
            UART2->C2 &= ~UART_C2_TIE_MASK;
            UART2->C2 &= ~UART_C2_TE_MASK;
        } else {
            UART2->D = send_buffer[send_ptr++];
        }
    }

    if ((UART2->S1 & UART_S1_RDRF_MASK) != 0U) {
        char rx_data = (char)UART2->D;
        recv_buffer[recv_ptr++] = rx_data;

        if ((rx_data == '\n') || (recv_ptr >= ((int)MAX_MSG_LEN - 1))) {
            TMessage msg;

            recv_buffer[recv_ptr] = '\0';
            strncpy(msg.message, recv_buffer, MAX_MSG_LEN);
            msg.message[MAX_MSG_LEN - 1U] = '\0';
            xQueueSendFromISR(queue, &msg, &hpw);
            portYIELD_FROM_ISR(hpw);
            recv_ptr = 0;
        }
    }
}

void parseUARTTask(void *p)
{
	(void) p;
    while (1) {
        TMessage msg;

        if (xQueueReceive(queue, &msg, portMAX_DELAY) == pdTRUE) {
            char plain_message[MAX_MSG_LEN];
            int s1 = 0;
            int s2 = 0;
            int s3 = 0;
            int rfid = 0;

            msg.message[strcspn(msg.message, "\r\n")] = '\0';
            PRINTF("UART RX encrypted: %s\r\n", msg.message);

            if (!decodeSecurePacket(msg.message, plain_message, sizeof(plain_message))) {
                PRINTF("Rejected invalid secure UART packet: %s\r\n", msg.message);
                continue;
            }

            PRINTF("UART RX decrypted: %s\r\n", plain_message);

            if (sscanf(plain_message, "%d,%d,%d,%d", &s1, &s2, &s3, &rfid) == 4) {
                current_speed_bands[0] = s1;
                current_speed_bands[1] = s2;
                current_speed_bands[2] = s3;
                authorized_rfid_request = (rfid == 1) ? 1 : 0;

                PRINTF("Updated Traffic Speeds -> R1: %d, R2: %d, R3: %d, RFID: %d\r\n",
                       current_speed_bands[0],
                       current_speed_bands[1],
                       current_speed_bands[2],
                       authorized_rfid_request);
            } else {
                PRINTF("Invalid decrypted UART message: %s\r\n", plain_message);
            }
        }
    }
}
