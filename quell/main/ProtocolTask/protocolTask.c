#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "protocolTask.h"
#include "protocol.h"
#include "fifo.h"
#include "quell.h"


#define PROTOCOL_UART_NUM UART_NUM_1
#define UART_BUF_SIZE (512UL)

#define FIFO_BUF_SIZE (128UL)
#define RX_READ_BUFFER_SIZE (32UL)


static const char *TAG = "protocol";
static QueueHandle_t uart_queue_rx;
static QueueHandle_t uart_queue_tx;

int32_t uartReceiveBytes(uint32_t _u32UartNumber, QueueHandle_t _xQueueRx, fifo_t *_psFIFORx, const char* _pcTAG)
{
    uart_event_t event;

    if(_xQueueRx == NULL || _psFIFORx == NULL || _pcTAG == NULL)
    {
        return QUELL_ERROR;
    }

    //Waiting for UART event.
    if(xQueueReceive(_xQueueRx, (void * )&event, (portTickType) 1/portTICK_PERIOD_MS)) 
    {
        //ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
        switch(event.type) 
        {
            //Event of UART receving data
            /*We'd better handler data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.*/
            case UART_DATA:
                //ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                while(event.size--)
                {
                    char cData;
                    if(uart_read_bytes(_u32UartNumber, &cData, 1, portMAX_DELAY) == 1)
                    {
                        if(FIFO_put(_psFIFORx, cData) == false)
                        {
                            ESP_LOGI(_pcTAG, "Failed to put byte in FIFO Rx");
                        }
                    }
                }
                //uart_write_bytes(EX_UART_NUM, (const char*) dtmp, event.size);
                break;
            //Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(_pcTAG, "hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(_u32UartNumber);
                xQueueReset(_xQueueRx);
                break;
            //Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(_pcTAG, "ring buffer full");
                // If buffer full happened, you should consider encreasing your buffer size
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(_u32UartNumber);
                xQueueReset(_xQueueRx);
                break;
            //Event of UART RX break detected
            case UART_BREAK:
                ESP_LOGI(_pcTAG, "uart rx break");
                break;
            //Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGI(_pcTAG, "uart parity error");
                break;
            //Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGI(_pcTAG, "uart frame error");
                break;
            //Others
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
        }
    }

    return QUELL_OK;
}

int32_t uartSendBytes(uint32_t _u32UartNumber, fifo_t *_psFIFOTx, const char* _pcTAG)
{
    char acSendBuffer[32];
    size_t u16FIFOCount;
    uint16_t u16Index;

    /* Check if there is data queued to be sent */
    if(FIFO_count(_psFIFOTx, &u16FIFOCount) == false || u16FIFOCount == 0)
    {
        return QUELL_ERROR;
    }

    /* Get the bytes and transfer to the local buffer to send to uart functions all at once */
    for(u16Index = 0; u16Index < u16FIFOCount || u16Index < sizeof(acSendBuffer); u16Index++)
    {
        if(FIFO_get(_psFIFOTx, (char*)&acSendBuffer[u16Index]) == false)
        {
            break;
        }
    }

    /* Send data to uart */
    if(uart_write_bytes(_u32UartNumber, (const char*) acSendBuffer, u16Index) != u16Index)
    {
        return QUELL_ERROR;
    }

    return QUELL_OK;
}


static void protocol_task(void *pvParameters)
{
    fifo_t sFIFORx;
    fifo_t sFIFOTx;
    char* pu8FIFORxBuffer = (char*) malloc(FIFO_BUF_SIZE);
    char* pu8FIFOTxBuffer = (char*) malloc(FIFO_BUF_SIZE);
    if(FIFO_init(&sFIFORx, pu8FIFORxBuffer, FIFO_BUF_SIZE) == false || FIFO_init(&sFIFOTx, pu8FIFOTxBuffer, FIFO_BUF_SIZE) == false)
    {
        ESP_LOGI(TAG, "Error initializing FIFO Rx or Tx");
        while(1);
    }

    for(;;) 
    {
        /* Transfer received bytes from uart to FIFO Rx */
        uartReceiveBytes(PROTOCOL_UART_NUM, uart_queue_rx, &sFIFORx, TAG);
        /* Transfer received bytes from FIFO Rx to uart*/
        uartSendBytes(PROTOCOL_UART_NUM, &sFIFOTx, TAG);

        /* Process incoming data */
        processIncomingCommunication(&sFIFORx, &sFIFOTx, TAG);

    }
    free(pu8FIFORxBuffer);
    pu8FIFORxBuffer = NULL;
    free(pu8FIFOTxBuffer);
    pu8FIFOTxBuffer = NULL;
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
    uart_driver_install(PROTOCOL_UART_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 20, &uart_queue_rx, 0);
    uart_param_config(PROTOCOL_UART_NUM, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(PROTOCOL_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Create Protocol task
    xTaskCreate(protocol_task, "protocol_task", 1024, NULL, 12, NULL);
}