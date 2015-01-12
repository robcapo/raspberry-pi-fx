#include "buffer.h"

void Push(buffer_t *buffer, item_t data) {
	// if front = rear and the buffer is not empty then overwrite what is on
    // the front, this allows use as a revolving circular buffer for receivers
	bool int_stat;
	int_stat = IntMasterDisable();
	if(buffer->front == buffer->rear && buffer->size) {
		buffer->front++;
		if(buffer->front > buffer->buffer_end)
			buffer->front = buffer->buffer_start;
		buffer->size--;
	}
	// push data onto rear location and increment rear
	*buffer->rear++ = data;
	// check to make sure rear isn't past the end of the buffer
	if(buffer->rear > buffer->buffer_end) buffer->rear = buffer->buffer_start;
	buffer->size++;
	if(buffer->Callback) buffer->Callback(buffer);
	if(!int_stat) IntMasterEnable();
}

item_t Pop(buffer_t *buffer) {
	item_t data;
	bool int_stat;
	int_stat = IntMasterDisable();
	data = *buffer->front;
	if(buffer->size != 0) {
		buffer->front++;
		if(buffer->front > buffer->buffer_end) buffer->front = buffer->buffer_start;
		buffer->size--;
	}
	if(!int_stat) IntMasterEnable();
	return data;
}

uint16_t GetSize(buffer_t *buffer) {
	return buffer->size;
}

void BufferInit(buffer_t *buffer, item_t *data_array, uint16_t max_size) {
	buffer->buffer_start = data_array;
        buffer->front = data_array;
	buffer->rear = data_array;
	buffer->buffer_end = data_array + max_size - 1;
	buffer->size = 0;
	buffer->Callback = 0;
}

void BufferSetCallback(buffer_t * buffer, void (*Callback)(buffer_t * buffer)) {
	buffer->Callback = (void (*)(void *))Callback; // cast callback to void pointer
}

void BufferClearCallback(buffer_t * buffer) { buffer->Callback = 0; }
