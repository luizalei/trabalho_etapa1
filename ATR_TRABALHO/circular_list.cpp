
#include "circular_list.h"


void cb_init(CircularList* cb) {
    cb->head = 0;
    cb->tail = 0;
    cb->full = false;
}

bool cb_is_empty(const CircularList* cb) {
    return (!cb->full && (cb->head == cb->tail));
}

bool cb_is_full(const CircularList* cb) {
    return cb->full;
}

bool cb_push(CircularList* cb, uint8_t data) {
    if (cb_is_full(cb)) return false;
    cb->data[cb->head] = data;
    cb->head = (cb->head + 1) % LIST_SIZE;
    cb->full = (cb->head == cb->tail);
    return true;
}

bool cb_pop(CircularList* cb, uint8_t* data) {
    if (cb_is_empty(cb)) return false;
    *data = cb->data[cb->tail];
    cb->tail = (cb->tail + 1) % LIST_SIZE;
    cb->full = false;
    return true;
}

size_t cb_size(const CircularList* cb) {
    if (cb_is_full(cb)) return LIST_SIZE;
    return (cb->head >= cb->tail) ? (cb->head - cb->tail) : (LIST_SIZE + cb->head - cb->tail);
}