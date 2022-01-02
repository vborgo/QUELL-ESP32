#ifndef _PROTOCOL_TASK_H_
#define _PROTOCOL_TASK_H_
#include "protocol.h"

void protocolTaskInit(void);
int32_t protocolInjectData(char* _pcData, uint16_t _u16DataLenght);

#endif /* _PROTOCOL_TASK_H_ */