#include "FIFOUart.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "FIFO.h"
#include "quell.h"

int32_t uartReceiveBytes(uint32_t _u32UartNumber, QueueHandle_t _xQueueRx, fifo_t *_psFIFORx, const char* _pcTAG)
{
    uart_event_t event;

    if(_xQueueRx == NULL || _psFIFORx == NULL || _pcTAG == NULL)
    {
        return QUELL_ERROR;
    }

    //Waiting for UART event.
    if(xQueueReceive(_xQueueRx, (void * )&event, 0)) 
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
                        //ESP_LOGI(_pcTAG, "%x", cData);
                        if(FIFO_put(_psFIFORx, cData) == false)
                        {
                            ESP_LOGI(_pcTAG, "Failed to put byte in FIFO Rx %u %u %u", _psFIFORx->head, _psFIFORx->tail, _psFIFORx->size);
                        }
                    }
                }
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
                ESP_LOGI(_pcTAG, "uart event type: %d", event.type);
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