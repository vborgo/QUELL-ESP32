#ifndef _FIFO_H_
#define _FIFO_H_

    #include <stdlib.h>
    #include <assert.h>
    #include <stdbool.h>
    typedef struct
    {
        size_t head;
        size_t tail;
        size_t size;
        char *buffer; //the buffer must have fifo_t.size + 1, once the tail is always empty in a circular buffer (same behaviour as '/0' in a string)
    }fifo_t;

    bool FIFO_init(fifo_t *fifo, char *buffer, size_t buffer_size);
    bool FIFO_get(fifo_t *fifo, char *dataout);
    bool FIFO_put(fifo_t *fifo, char datain);
    bool FIFO_peak(fifo_t *fifo, size_t pos, char *dataout);
    bool FIFO_count(fifo_t *fifo, size_t *count);
    bool FIFO_free(fifo_t *fifo, size_t *free);
    bool FIFO_clean(fifo_t *fifo);

#endif /* _FIFO_H_ */