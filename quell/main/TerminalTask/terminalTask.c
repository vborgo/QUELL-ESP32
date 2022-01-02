#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "terminalTask.h"
#include "FIFO.h"
#include "quell.h"
#include "FIFOUart.h"
#include "terminal.h"


#define TERMINAL_UART_NUM UART_NUM_0
#define UART_BUF_SIZE (512UL)

#define FIFO_BUF_SIZE (128UL)
#define RX_READ_BUFFER_SIZE (32UL)


static const char *TAG = "terminal";
static QueueHandle_t uart_queue_rx;

static void terminal_task(void *pvParameters)
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
        uartReceiveBytes(TERMINAL_UART_NUM, uart_queue_rx, &sFIFORx, TAG);
        /* Transfer received bytes from FIFO Rx to uart*/
        uartSendBytes(TERMINAL_UART_NUM, &sFIFOTx, TAG);

        /* Process Terminal */
        processTerminal(&sFIFORx, &sFIFOTx, TAG);

    }
    free(pu8FIFORxBuffer);
    pu8FIFORxBuffer = NULL;
    free(pu8FIFOTxBuffer);
    pu8FIFOTxBuffer = NULL;
    vTaskDelete(NULL);
}



void terminalTaskInit(void)
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
    uart_driver_install(TERMINAL_UART_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 20, &uart_queue_rx, 0);
    uart_param_config(TERMINAL_UART_NUM, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(TERMINAL_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Create Protocol task
    xTaskCreate(terminal_task, "terminal_task", 4096, NULL, 0, NULL);
}