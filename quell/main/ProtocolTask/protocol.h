#ifndef _TERMIANL_TASK_H_
#define _TERMINAL_TASK_H_

#include <stdio.h>
#include <string.h>
#include "FIFO.h"

#define MINIMUM_PACKET_SIZE 7
#define PACKE_SIZE(msg_lenght) (MINIMUM_PACKET_SIZE + msg_lenght)
#define MESSAGE_SIZE(packet_length) (packet_length - MINIMUM_PACKET_SIZE)

int32_t processIncomingCommunication(fifo_t *_psFIFORx, fifo_t *_psFIFOTx, const char* _pcTAG);
int32_t makePacket(uint8_t * _pu8PacketBuffer, uint16_t _u16PacketBufferSize, uint8_t * _pu8Message, uint16_t _u16MessageSize);

#endif /* _TERMINAL_TASK_H_ */