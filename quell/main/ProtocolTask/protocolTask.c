#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "protocolTask.h"

static const char *TAG = "protocol";
#define EX_UART_NUM UART_NUM_1

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart_queue;


#define QUELL_OK (0)
#define QUELL_ERROR (-1)


#define SOH 1
#define SOT 2
#define EOT 3
#define MINIMUM_PACKET_SIZE 7

/*
* One of the ways to calculate CRC16
* From: https://stackoverflow.com/questions/17196743/crc-ccitt-implementation
*/
static uint16_t calculateCRC16CCITT(char *ptr, int16_t count)
{
   int  crc;
   char i;
   crc = 0;

   if(ptr == NULL || count == 0)
   {
       return 0;
   }

   while (--count >= 0)
   {
      crc = crc ^ (int) *ptr++ << 8;
      i = 8;
      do
      {
         if (crc & 0x8000)
            crc = crc << 1 ^ 0x1021;
         else
            crc = crc << 1;
      } while(--i);
   }
   return (crc);
}

static int32_t calculatePacketCRC16(uint16_t *_pu16CRC16, uint8_t *_pu8Packet, uint16_t _u16PacketSize)
{
    if(_pu16CRC16 == NULL || _pu8Packet == NULL || _u16PacketSize)
    {
        return QUELL_ERROR;
    }

    /* Calculate the CRC16, eliminating the CRC16 bytes (of course) */
    *_pu16CRC16 = calculateCRC16CCITT((char*)_pu8Packet, _u16PacketSize - 2);

    return QUELL_OK;
}

/*
    PACKET DESCRIPTION (Big Endian)

    PACKET ITEM:            LENGTH:             DESCRIPTION:                    CONST VALUE:
    SOH                     u8                  Start of heading                0X01
    Packet Size             u16                 Packet size                     NO
    SOT                     u8                  Start of text                   0x02
    Message                 Variable            Message content                 NO
    ETX                     u8                  End of Text                     0x03
    CRC16                   u16                 CRC16 of header + message       NO
*/
static int32_t verifyProtocolPacket(uint8_t *_pu8Packet, uint16_t _u16PacketSize)
{
    uint16_t u16CalculatedCRC16 = 0;

    if(_pu8Packet == NULL || _u16PacketSize)
    {
        return QUELL_ERROR;
    }

    /* A packet needs at least 7 bytes to be valid */
    if(_u16PacketSize < MINIMUM_PACKET_SIZE)
    {
        return QUELL_ERROR;
    }

    /* Check the const bytes (SOH, SOT and )  */
    if(_pu8Packet[0] != SOH || _pu8Packet[3] != SOH || _pu8Packet[_u16PacketSize - 3] != EOT)
    {
        return QUELL_ERROR;
    }

    /*Check if the message size is correct*/
    if(*((uint16_t*)(&_pu8Packet[1])) != _u16PacketSize - MINIMUM_PACKET_SIZE) //Maybe have created a union to get the u16 from the packet array without crazy casts would have been a good idea
    {
        return QUELL_ERROR;
    }

    /* Checks CRC16 */
    if(calculatePacketCRC16(&u16CalculatedCRC16, _pu8Packet, _u16PacketSize) == QUELL_ERROR || u16CalculatedCRC16 != *((uint16_t*)(&_pu8Packet[_u16PacketSize - 2])))
    {
        return QUELL_ERROR;
    }

    return QUELL_OK;
}

static int32_t extractMessageFromPacket(uint8_t *_pu8Packet, uint16_t _u16PacketSize, uint8_t *_pu8Message, uint16_t *_u16MessageSize)
{
    if(_pu8Packet == NULL || _u16PacketSize)
    {
        return QUELL_ERROR;
    }

    /* A packet needs at least 7 bytes to be valid */
    if(_u16PacketSize < MINIMUM_PACKET_SIZE)
    {
        return QUELL_ERROR;
    }

    /* Points to start of the message */
    _pu8Message = &_pu8Packet[4];
    (void)_pu8Message;

    /* Gets the message size */
    *_u16MessageSize = *((uint16_t*)(&_pu8Packet[1]));
    

    return QUELL_OK;
}

static void protocol_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    for(;;) 
    {
        //Waiting for UART event.
        if(xQueueReceive(uart_queue, (void * )&event, (portTickType)portMAX_DELAY)) 
        {
            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
            switch(event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    ESP_LOGI(TAG, "[DATA EVT]:");
                    //uart_write_bytes(EX_UART_NUM, (const char*) dtmp, event.size);
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart_queue);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(TAG, "uart rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;
                //Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}



void protocolTaskInit(void)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    //Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Set uart pattern detect function.
    //uart_enable_pattern_det_baud_intr(EX_UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
    //Reset the pattern queue length to record at most 20 pattern positions.
    //uart_pattern_queue_reset(EX_UART_NUM, 20);

    //Create Protocol task
    xTaskCreate(protocol_task, "protocol_task", 1024, NULL, 12, NULL);
}