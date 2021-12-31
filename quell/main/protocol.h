#ifndef _TERMIANL_TASK_H_
#define _TERMINAL_TASK_H_

#include <stdio.h>
#include <string.h>
#include "FIFO.h"

int32_t processIncomingCommunication(fifo_t *_psFIFORx, fifo_t *_psFIFOTx, char* _pcTAG);

#endif /* _TERMINAL_TASK_H_ */