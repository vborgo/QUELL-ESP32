#ifndef _FIFOUART_H_
#define _FIFOUART_H_

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "FIFO.h"

int32_t uartReceiveBytes(uint32_t _u32UartNumber, QueueHandle_t _xQueueRx, fifo_t *_psFIFORx, const char* _pcTAG);
int32_t uartSendBytes(uint32_t _u32UartNumber, fifo_t *_psFIFOTx, const char* _pcTAG);

#endif /* _FIFOUART_H_ */