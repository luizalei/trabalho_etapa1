#pragma once
// circular_buffer.h
#ifndef CIRCULAR_LIST_H
#define CIRCULAR_LIST_H

#include <stdint.h>
#include <stdbool.h>

#define LIST_SIZE 200

typedef struct {
    uint8_t data[LIST_SIZE];
    uint16_t head;
    uint16_t tail;
    bool full;
} CircularList;

// Protótipos das funções
void cb_init(CircularList* cb);
bool cb_is_empty(const CircularList* cb);
bool cb_is_full(const CircularList* cb);
bool cb_push(CircularList* cb, uint8_t data);
bool cb_pop(CircularList* cb, uint8_t* data);
size_t cb_size(const CircularList* cb);

#endif