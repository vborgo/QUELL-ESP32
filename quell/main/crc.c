#include <stdio.h>
#include <string.h>
#include "crc.h"

/*
* One of the ways to calculate CRC16
* From: https://stackoverflow.com/questions/17196743/crc-ccitt-implementation
*/
uint16_t calculateCRC16CCITT(char *ptr, int16_t count)
{
   int  crc;
   char i;
   crc = 0;

   if(ptr == NULL || count == 0)
   {
       return 0;
   }

   while (--count >= 0)
   {
      crc = crc ^ (int) *ptr++ << 8;
      i = 8;
      do
      {
         if (crc & 0x8000)
            crc = crc << 1 ^ 0x1021;
         else
            crc = crc << 1;
      } while(--i);
   }
   return (crc);
}