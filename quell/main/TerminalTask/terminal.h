#ifndef _TERMINAL_H_
#define _TERMINAL_H_
#include <stdio.h>
#include <string.h>
#include "FIFO.h"

int32_t processTerminal(fifo_t *_psFIFORx, fifo_t *_psFIFOTx, char* _pcTAG);

#endif /* _TERMINAL_H_ */