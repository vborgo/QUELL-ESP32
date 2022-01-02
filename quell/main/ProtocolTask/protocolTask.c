#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "protocolTask.h"
#include "protocol.h"
#include "FIFO.h"
#include "quell.h"
#include "FIFOUart.h"


#define PROTOCOL_UART_NUM UART_NUM_1
#define UART_BUF_SIZE (512UL)

#define FIFO_BUF_SIZE (128UL)
#define RX_READ_BUFFER_SIZE (32UL)

#define PROTOCOL_QUEUE_SIZE (128UL)


static const char *TAG = "protocol";
static QueueHandle_t uart_queue_rx;
QueueHandle_t tQueueProtocol;

//@todo: It would be a perfect idea to inject as a whole message or packet, and use the FreeRTOS Queue to send a message struct. Much better organization of the code.
int32_t protocolInjectData(char* _pcData, uint16_t _u16DataLenght)
{
    if(_pcData == NULL || _u16DataLenght == 0)
    {
        return QUELL_ERROR;
    }

    for(uint16_t u16Index = 0; u16Index < _u16DataLenght; u16Index++)
    {
        xQueueSend(tQueueProtocol, (void *)&_pcData[u16Index], 0);
    }

    return QUELL_OK;
}

static int32_t protocolTransferInjectedDataToFIFO(fifo_t *_psFIFOTx)
{
    char cData;
    size_t u16FIFOCount;

    if(_psFIFOTx == NULL)
    {
        return QUELL_ERROR;
    }

    if(FIFO_count(_psFIFOTx, &u16FIFOCount) == false || u16FIFOCount > 0)
    {
        // It is protecting to not conflict other packets
        return QUELL_ERROR; //That is why you should have created the queue with FreeRTOS queue and a message struct (this protection should not exist)
    }

    if (xQueueReceive(tQueueProtocol, (void*)&cData, 0) == pdTRUE)
    {
        FIFO_put(_psFIFOTx, cData);
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

        /* Transfer injected packet to FIFO */
        protocolTransferInjectedDataToFIFO(&sFIFOTx);

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
    uart_set_pin(PROTOCOL_UART_NUM, 4, 5, 18, 19);

    //Create Protocol queue (to inject messages from other tasks to go out through uart) @todo: Make the others FIFOs from FreeRTOS Queues 
    tQueueProtocol = xQueueCreate(PROTOCOL_QUEUE_SIZE, sizeof(char));

    //Create Protocol task
    xTaskCreate(protocol_task, "protocol_task", 4096, NULL, 0, NULL);
}