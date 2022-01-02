#include "protocol.h"
#include "quell.h"
#include "FIFO.h"
#include "esp_log.h"
#include "crc.h"

#define SOH 1
#define SOT 2
#define EOT 3

#define MESSAGE_MARCO "marco"
#define MESSAGE_POLO "polo"
#define MESSAGE_OK  "ok"
#define MESSAGE_ERROR "error"


int32_t calculatePacketCRC16(uint16_t *_pu16CRC16, uint8_t *_pu8Packet, uint16_t _u16PacketSize)
{
    if(_pu16CRC16 == NULL || _pu8Packet == NULL || _u16PacketSize == 0)
    {
        return QUELL_ERROR;
    }

    /* Calculate the CRC16, eliminating the CRC16 bytes (of course) */
    *_pu16CRC16 = calculateCRC16CCITT((char*)_pu8Packet, _u16PacketSize - 2);

    //ESP_LOGI("terminal", "CRC %x %x", *_pu16CRC16, _u16PacketSize);

    return QUELL_OK;
}

/*
    PACKET DESCRIPTION (Big Endian)

    PACKET ITEM:            LENGTH:             DESCRIPTION:                    CONST VALUE:
    SOH                     u8                  Start of heading                0X01
    Packet Size             u16                 Packet size                     NO
    SOT                     u8                  Start of text                   0x02
    Message                 Variable            Message content                 NO
    EOT                     u8                  End of Text                     0x03
    CRC16                   u16                 CRC16 of header + message       NO
*/
int32_t verifyPacket(uint8_t *_pu8Packet, uint16_t _u16PacketSize)
{
    uint16_t u16CalculatedCRC16 = 0;

    if(_pu8Packet == NULL || _u16PacketSize == 0)
    {
        return QUELL_ERROR;
    }

    /* A packet needs at least 7 bytes to be valid */
    if(_u16PacketSize < MINIMUM_PACKET_SIZE)
    {
        return QUELL_ERROR;
    }

    /* Check the const bytes (SOH, SOT and )  */
    if(_pu8Packet[0] != SOH || _pu8Packet[3] != SOT || _pu8Packet[(_u16PacketSize - 1) - 2] != EOT)
    {
        return QUELL_ERROR;
    }

    /*Check if the message size is correct*/
    if((((uint16_t)_pu8Packet[1] << 8) | (uint16_t)_pu8Packet[2]) != _u16PacketSize)
    {
        return QUELL_ERROR;
    }

    /* Checks CRC16 */
    if(calculatePacketCRC16(&u16CalculatedCRC16, _pu8Packet, _u16PacketSize) == QUELL_ERROR || 
       u16CalculatedCRC16 != (((uint16_t)_pu8Packet[_u16PacketSize - 2] << 8) | (uint16_t)_pu8Packet[_u16PacketSize - 1]))
    {
        return QUELL_ERROR;
    }

    return QUELL_OK;
}


int32_t extractMessageFromPacket(uint8_t *_pu8Packet, uint16_t _u16PacketSize, uint8_t *_pu8Message, uint16_t *_pu16MessageSize)
{
    if(_pu8Packet == NULL || _u16PacketSize == 0 || _pu8Message == NULL || _pu16MessageSize == NULL)
    {
        return QUELL_ERROR;
    }

    /* A packet needs at least 7 bytes to be valid */
    if(_u16PacketSize < MINIMUM_PACKET_SIZE)
    {
        return QUELL_ERROR;
    }

    /* Gets the message size */
    *_pu16MessageSize = MESSAGE_SIZE((((uint16_t)_pu8Packet[1] << 8) | (uint16_t)_pu8Packet[2]));

    /* Copies the message */
    memcpy(_pu8Message, &_pu8Packet[4], *_pu16MessageSize);
    _pu8Message[*_pu16MessageSize] = 0;
    
    return QUELL_OK;
}

int32_t trimFIFOForPacket(fifo_t *_psFIFORx)
{
    char cData;
    size_t tFIFOCount;

    if(_psFIFORx == NULL)
    {
        return QUELL_ERROR;
    }

    while(FIFO_count(_psFIFORx, &tFIFOCount) == true && tFIFOCount > 0)
    {
        if(FIFO_peak(_psFIFORx, 0, &cData) == true && cData == SOH)
        {
            break;
        }
        else
        {
            /* Remove trash before a valid packet */
            if (FIFO_get(_psFIFORx,&cData) == false)
            {
                return QUELL_ERROR;
            }
        }
    }

    return QUELL_OK;
}
int32_t getPacketFromFIFO(fifo_t *_psFIFORx, uint8_t *_pu8Buffer, uint16_t _u16BufferSize, uint16_t *_pu16PacketSize)
{
    char cData;
    size_t tCount = 0;
    uint16_t u16PacketSize = 0;

    if(_psFIFORx == NULL || _pu8Buffer == NULL || _u16BufferSize == 0 || _pu16PacketSize == NULL)
    {
        return QUELL_ERROR;
    }

    /* Zero the size */
    *_pu16PacketSize = 0;
    

    /* Trim the FIFO for eventual garbage */
    if(trimFIFOForPacket(_psFIFORx) == QUELL_ERROR)
    {
        return QUELL_ERROR;
    }

    /* Check if there is at least the smallest amount of data for the packet */
    if(FIFO_count(_psFIFORx, &tCount) == false || tCount < MINIMUM_PACKET_SIZE)
    {
        return QUELL_ERROR;
    }

    /* Get the size from the header of the packet that should be already stored in FIFO */
    if(FIFO_peak(_psFIFORx, 1, &cData) == true)
    {
        u16PacketSize = ((uint16_t)cData) << 8;
        if(FIFO_peak(_psFIFORx, 2, &cData) == true)
        {
            u16PacketSize |= ((uint16_t)cData) & 0xFF;
        }
    }

    /* If the amount in FIFO is smaller than the packet size, it means that it is not a full packet yet */
    if(tCount < u16PacketSize)
    {
        return QUELL_ERROR;
    }

    /* Just a last check of one of the known bytes from the packet */
    if(FIFO_peak(_psFIFORx, (u16PacketSize - 1) - 2, &cData) == false || cData != EOT)
    {
        return QUELL_ERROR;
    }

    /* Get the packet size */
    *_pu16PacketSize = u16PacketSize;

    /* At this point, there is a full packet */
    tCount = 0;
    while(u16PacketSize-- > 0)
    {
        /* Protect agains overflow */
        if(tCount >= _u16BufferSize)
        {
            return QUELL_ERROR;
        }

        /* Transfer the packet to buffer */
        if(FIFO_get(_psFIFORx, (char*)&_pu8Buffer[tCount++]) == false)
        {
            return QUELL_ERROR;
        }
    }

    return QUELL_OK;
}

int32_t makePacket(uint8_t * _pu8PacketBuffer, uint16_t _u16PacketBufferSize, uint8_t * _pu8Message, uint16_t _u16MessageSize)
{
    uint16_t CRC16 = 0;

    if(_pu8PacketBuffer == NULL || 
      _u16PacketBufferSize < MINIMUM_PACKET_SIZE + _u16MessageSize || 
      _pu8Message == NULL || 
      _u16MessageSize == 0)
    {
        return QUELL_ERROR;
    }

    //ESP_LOGI("terminal", "-1<%u> <%u>", _u16PacketBufferSize, MINIMUM_PACKET_SIZE + _u16MessageSize);

    /* Start of heading*/
    _pu8PacketBuffer[0] = SOH;
    /* Packet Size */
    _pu8PacketBuffer[1] = ((MINIMUM_PACKET_SIZE + _u16MessageSize) >> 8) & 0xFF;
    _pu8PacketBuffer[2] = (MINIMUM_PACKET_SIZE + _u16MessageSize) & 0xFF;
    /* Start of Text */
    _pu8PacketBuffer[3] = SOT;
    /* Message */
    memcpy(&_pu8PacketBuffer[4], _pu8Message, _u16MessageSize);
    /* End of Text */
    _pu8PacketBuffer[4 + _u16MessageSize] = EOT;

    /* Caculate CRC16 */
    if(calculatePacketCRC16(&CRC16, _pu8PacketBuffer, MINIMUM_PACKET_SIZE + _u16MessageSize) == QUELL_ERROR)
    {
        return QUELL_ERROR;
    }

    //ESP_LOGI("terminal", "-2<%u> <%u> <%x>", _u16PacketBufferSize, MINIMUM_PACKET_SIZE + _u16MessageSize, CRC16);


    _pu8PacketBuffer[4 + _u16MessageSize + 1] = (CRC16 >> 8) & 0xFF;
    _pu8PacketBuffer[4 + _u16MessageSize + 2] = CRC16 & 0xFF;

    return QUELL_OK;
}

int32_t sendMessage(fifo_t *_psFIFOTx, uint8_t * _pu8Message, uint16_t _u16MessageSize)
{
    uint8_t au8PacketBuffer[64];

    if(_psFIFOTx == NULL || _pu8Message == NULL || _u16MessageSize == 0)
    {
        return QUELL_ERROR;
    }

    /* Check if the buffer has enough space (this buffer could be dinamic, but static buffer makes it more predictable) */
    if(MINIMUM_PACKET_SIZE + _u16MessageSize > sizeof(au8PacketBuffer))
    {
        return QUELL_ERROR;
    }

    /* Make the packet to send */
    if(makePacket(au8PacketBuffer, sizeof(au8PacketBuffer), _pu8Message, _u16MessageSize) == QUELL_OK)
    {
        /* Send the packet */
        for(uint16_t u16Index = 0; u16Index < MINIMUM_PACKET_SIZE + _u16MessageSize; u16Index++)
        {
            /* Place the packet in FIFO */
            if(FIFO_put(_psFIFOTx, au8PacketBuffer[u16Index]) == false)
            {
                return QUELL_ERROR;
            }
        }
    }

    return QUELL_OK;
}



int32_t acknowledgeMessage(fifo_t *_psFIFOTx, uint8_t * _pu8Message, uint16_t _u16MessageSize, const char* _pcTAG)
{
    struct
    {
    char *pu8ReceivedMessage;
    char *pu8AknowledgementMessage;
    }sMessageAnswer[] = {{MESSAGE_MARCO, MESSAGE_POLO}, 
                         {MESSAGE_POLO, MESSAGE_OK}, 
                         {MESSAGE_OK, NULL},
                         {MESSAGE_ERROR, NULL},
                         {NULL, NULL}};

    if(_psFIFOTx == NULL || _pu8Message == NULL || _u16MessageSize == 0)
    {
        return QUELL_ERROR;
    }

    for(uint16_t u16Index = 0; ; u16Index++)
    {
        /* Check if the message received is known */
        if(sMessageAnswer[u16Index].pu8ReceivedMessage != NULL)
        {
            /* Find which message */
            if(strcmp((char*)_pu8Message, (char*)sMessageAnswer[u16Index].pu8ReceivedMessage) == 0)
            {
                if(_pcTAG != NULL)
                {
                    ESP_LOGI(_pcTAG, "Msg Rx: %s", sMessageAnswer[u16Index].pu8ReceivedMessage);
                }

                /*Check if the message received needs aknowledgement*/
                if(sMessageAnswer[u16Index].pu8AknowledgementMessage != NULL)
                {
                    if(_pcTAG != NULL)
                    {
                        ESP_LOGI(_pcTAG, "Msg Tx: %s", sMessageAnswer[u16Index].pu8AknowledgementMessage);
                    }

                    /* Acknowledge the message */
                    return (sendMessage(_psFIFOTx, (uint8_t*)sMessageAnswer[u16Index].pu8AknowledgementMessage, strlen((char*)sMessageAnswer[u16Index].pu8AknowledgementMessage)));
                }
                else
                {
                    /* The message doesnt have/need aknowledgement */
                    return QUELL_OK;
                }
            }
        }
        /* The message received is unkwonw */
        else
        {
            return (sendMessage(_psFIFOTx, (uint8_t*)MESSAGE_ERROR, strlen(MESSAGE_ERROR)));
        }
    }


    return QUELL_ERROR;
}

int32_t processIncomingCommunication(fifo_t *_psFIFORx, fifo_t *_psFIFOTx, const char* _pcTAG)
{
    uint8_t au8PacketBuffer[64];
    uint8_t au8MessageBuffer[64];
    uint16_t u16PacketSize;
    uint16_t u16MessageSize;

    if(_psFIFORx == NULL || _psFIFOTx == NULL)
    {
        return QUELL_ERROR;
    }
    

    /* Get packet from fifo Rx */
    if(getPacketFromFIFO(_psFIFORx, au8PacketBuffer, sizeof(au8PacketBuffer), &u16PacketSize) == QUELL_OK)
    {
        /* When a confirmed packet arrived, check it */
        if(verifyPacket(au8PacketBuffer, u16PacketSize) == QUELL_OK)
        {
            /*Everything ok, extract the packet*/
            if(extractMessageFromPacket(au8PacketBuffer, u16PacketSize, au8MessageBuffer, &u16MessageSize) == QUELL_OK)
            {
                /* Acknowledge message received*/
                return (acknowledgeMessage(_psFIFOTx, au8MessageBuffer, u16MessageSize, _pcTAG));
            }
        }
    }

    return QUELL_ERROR;
}