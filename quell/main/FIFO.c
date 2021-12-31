#include "FIFO.h"

bool FIFO_init(fifo_t *fifo, char *buffer, size_t buffer_size)
{
    if (fifo == NULL || buffer == NULL || buffer_size == 0) //checks the pointers
        return false;

    fifo->buffer = buffer;
    fifo->size = buffer_size;

    return true;
}

bool FIFO_put(fifo_t *fifo, char datain)
{
    if (fifo == NULL || datain == NULL) //checks the pointers
        return false;

    if (((fifo->tail + 1) % fifo->size) == fifo->head) //checks if the next position isnt the tail, which means that the fifo is full
        return false;

    fifo->buffer[fifo->tail] = datain; //adds the new word
    fifo->tail = (fifo->tail + 1) % fifo->size; //updates the head position
    return true;
}

bool FIFO_get(fifo_t *fifo, char *dataout)
{
    if (fifo == NULL || dataout == NULL) //checks the pointers
        return false;

    if (fifo->tail == fifo->head) //checks if there is any word in the fifos buffer
        return false;

    *dataout = fifo->buffer[fifo->head]; //gets the data
    //fifo->data[fifo->head] = 0; ->clear the space may not be the best idea for debugging
    fifo->head = (fifo->head + 1) % fifo->size;//updates the tail position
    return true;
}


bool FIFO_peak(fifo_t *fifo, size_t pos, char *dataout) //To analyse data without removing from fifo (pos = 0 is the head position)
{
    if (fifo == NULL || dataout == NULL) //checks the pointers
            return false;

   if (fifo->head == fifo->tail)
    return false;


   if(fifo->head > fifo->tail)
   {
        if(fifo->head + pos > fifo->size - 1)
        {
          pos = (fifo->head + pos) % fifo->size;

          if(pos > fifo->tail)
            return false;
        }
        else
        {
          pos = fifo->head + pos;
        }
   }
   else
   {
      pos = fifo->head + pos;
      if(pos >= fifo->tail)
        return false;
   }

   *dataout = fifo->buffer[pos];
   return true;
}

bool FIFO_count(fifo_t *fifo, size_t *count) //Returns occupied space
{
    if (fifo == NULL) //checks the pointer
        return false;

    if (fifo->head <= fifo->tail)
      *count = fifo->tail - fifo->head;
    else
      *count = fifo->size - (fifo->head - fifo->tail);

    return true;
}

bool FIFO_free(fifo_t *fifo, size_t *free) //Returns free space
{
    if (fifo == NULL) //checks the pointer
        return false;

    if (fifo->head <= fifo->tail)
      *free = fifo->size - (fifo->tail - fifo->head) - 1;
    else
      *free = (fifo->head - fifo->tail) - 1;

    return true;
}

bool FIFO_clean(fifo_t *fifo) //Cleans the fifo
{
    if (fifo == NULL) //checks the pointer
        return false;

    fifo->head = fifo->tail; //clears the occupied data in the buffer

    return true;
}