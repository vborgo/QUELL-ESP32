#include "terminal.h"
#include "quell.h"
#include "protocolTask.h"
#include "FIFO.h"
#include "esp_log.h"
#include "crc.h"
#define _TERMINAL_MAX_ARGS 10

typedef struct
{
	char *pcCommand;
	int32_t(*fpFunction)(uint16_t _u8Argc, char **_ppcArgv, void* _internalArgs);
	char *pcArguments;
	char *pcComment;
} s_terminal_commands_t;

static int32_t terminal_sendMarco(uint16_t _u8Argc, char **_ppcArgv, void* _internalArgs);
static int32_t terminal_help(uint16_t _u8Argc, char **_ppcArgv, void* _internalArgs);
static int32_t  terminal_crc16(uint16_t _u8Argc, char **_ppcArgv, void* _internalArgs);


s_terminal_commands_t asTerminalCommands[] = {
                                             { "marco", &terminal_sendMarco,	    " ",	    "Send MARCO on protocol uart"},
                                             { "help",  &terminal_help, 			" ",        "Help"},
                                             { "?",     &terminal_help, 			" ",        "Help"},
                                             { "crc",   &terminal_crc16,            "<string>", "CRC16-CCITT(XMODEM)"},
                                             { NULL,    NULL,                    NULL,   NULL}
                                             };

static int32_t  terminal_crc16(uint16_t _u8Argc, char **_ppcArgv, void* _internalArgs)
{
    if(_u8Argc < 2)
    {
        return QUELL_ERROR;
    }
    
    if(_internalArgs != NULL)
    {
        fifo_t *psFIFOTx = (fifo_t *)_internalArgs;

        FIFO_printf(psFIFOTx, "str:%s size:0x%x crc:0x%x\n", _ppcArgv[1], strlen(_ppcArgv[1]), calculateCRC16CCITT(_ppcArgv[1], strlen(_ppcArgv[1])));
    }

    return QUELL_OK;
}

static int32_t  terminal_sendMarco(uint16_t _u8Argc, char **_ppcArgv, void* _internalArgs)
{
    char acPacketBuffer[32];
    char* pcMarco = "marco";

            
    /* Make the packet and send for the protocol task to send trought uart */
    if(makePacket((uint8_t*)acPacketBuffer, sizeof(acPacketBuffer), (uint8_t*)pcMarco, strlen(pcMarco)) == QUELL_OK)
    {
        if(_internalArgs != NULL)
        {
            fifo_t *psFIFOTx = (fifo_t *)_internalArgs;

            FIFO_printf(psFIFOTx, "Msg Tx: %s\n", pcMarco); //To match and ackownledge like the rest of the debug
        }
        return (protocolInjectData(acPacketBuffer, PACKE_SIZE(strlen(pcMarco))));
    }

    //ESP_LOGI("terminal", "MARCOOOO");
    return QUELL_ERROR;
}

static int32_t terminal_help(uint16_t _u8Argc, char **_ppcArgv, void* _internalArgs)
{
    fifo_t *psFIFOTx;

    if(_internalArgs != NULL)
    {
        psFIFOTx = (fifo_t *)_internalArgs;

        /* Print the help menu on terminal */
        ESP_LOGI("terminal", "Help");
        //FIFO_printf(psFIFOTx, "Help\n");
        for(uint16_t u16Index = 0; asTerminalCommands[u16Index].pcCommand != NULL; u16Index++)
        {
            //There is some mess here, @todo: sort out (impossible without JTAG)
            //FIFO_printf(psFIFOTx, "- %s %s %s\r\n", asTerminalCommands[u16Index].pcCommand, asTerminalCommands[u16Index].pcArguments, asTerminalCommands[u16Index].pcComment);
            ESP_LOGI("terminal", "- %s %s %s", asTerminalCommands[u16Index].pcCommand, asTerminalCommands[u16Index].pcArguments, asTerminalCommands[u16Index].pcComment);
        }
    }

    return QUELL_OK;
}

static int32_t terminal_executeCommand(uint16_t _pu16Argc, char **_ppcArgv, void* _internalArgs)
{
	if(_ppcArgv == NULL || _pu16Argc == 0)
	{
		return QUELL_ERROR;
	}

	/* Run through the list */
	for(uint16_t u16Index = 0; asTerminalCommands[u16Index].pcCommand != NULL; u16Index++)
	{
		/* Tries to find the command in the list */
        //ESP_LOGI("terminal", "<%s><%s>", asTerminalCommands[u16Index].pcCommand, _ppcArgv[0]);
		if(strcmp(asTerminalCommands[u16Index].pcCommand, _ppcArgv[0]) == 0)
		{
			if(asTerminalCommands[u16Index].fpFunction == NULL)
			{
				return QUELL_ERROR;
			}

			/* Call the callback function for the command like a batch file */
			return(asTerminalCommands[u16Index].fpFunction(_pu16Argc, _ppcArgv, _internalArgs));
		}
	}

	return QUELL_ERROR;
}

static int32_t terminal_splitArgs(char *_pcBuffer, uint16_t *_pu16Argc, char **_ppcArgv, uint16_t _u16MaximumArgs)
{
	char *cPointerHandle;


	/* Protects on the received arguments */
	if(_pcBuffer == NULL || _pu16Argc == NULL || _ppcArgv == NULL || _u16MaximumArgs == 0)
	{
		return QUELL_ERROR;
	}

    /* Zero the arguments */
	*_pu16Argc = 0;

	/* Gets the splitted string */
    
	cPointerHandle = strtok(_pcBuffer, (const char*)" ");
	while (cPointerHandle != NULL && *_pu16Argc < _u16MaximumArgs)
	{
        //ESP_LOGI("terminal", "<%s>", cPointerHandle);
		_ppcArgv[*_pu16Argc] = cPointerHandle;
		(*_pu16Argc)++;

		cPointerHandle = strtok(NULL, (const char*)" ");
	}

	return QUELL_OK;
}

int32_t processCommand(char* _pcCommand, void* _internalArgs)
{
    char* apcArgv[_TERMINAL_MAX_ARGS];
	uint16_t u16Argc;

    if(_pcCommand == NULL)
    {
        return QUELL_ERROR;
    }

    if(terminal_splitArgs(_pcCommand, &u16Argc, apcArgv, _TERMINAL_MAX_ARGS) == QUELL_OK)
	{
		return(terminal_executeCommand(u16Argc, apcArgv, _internalArgs));
	}

	return QUELL_ERROR;
}

int32_t processTerminal(fifo_t *_psFIFORx, fifo_t *_psFIFOTx, char* _pcTAG)
{
    static char acCommandBuffer[64];
    static uint16_t u16CommandBufferIndex = 0;
    char cData;

    if(_psFIFORx == NULL || _psFIFOTx == NULL)
    {
        return QUELL_ERROR;
    }
    
    /* Process incoming byte */
    if(FIFO_get(_psFIFORx, &cData) == true)
    {
        if(cData == '\r' || cData == '\n')
        {
            acCommandBuffer[u16CommandBufferIndex] = 0;
            
            u16CommandBufferIndex = 0;
            //ESP_LOGI(_pcTAG, "<%s>", acCommandBuffer);
            if(processCommand(acCommandBuffer, (void*)_psFIFOTx) == QUELL_OK)
            {
                FIFO_printf(_psFIFOTx, "Executed <%s>\r\n", acCommandBuffer);
                return QUELL_OK;
            }
        }
        else if (u16CommandBufferIndex < sizeof(acCommandBuffer) - 1)
        {
            acCommandBuffer[u16CommandBufferIndex++] = cData;
        }
    }

    return QUELL_ERROR;
}