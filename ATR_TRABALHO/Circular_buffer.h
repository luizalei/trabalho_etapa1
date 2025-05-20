#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 200
#define MAX_MSG_LENGTH 41  // 40 chars + null terminator (para a mensagem maior)
#define SMALL_MSG_LENGTH 35 // 34 chars + null terminator

// Estrutura da lista circular
typedef struct {
    char data[BUFFER_SIZE][MAX_MSG_LENGTH];
    int head;
    int tail;
    int count;
    CRITICAL_SECTION cs;  // Seção crítica para proteger o buffer
} CircularBuffer;

// Variável global do buffer
extern CircularBuffer circularBuffer;

void InitializeBuffer();
void DestroyBuffer();
void WriteToBuffer(const char* value, size_t msg_size); 
int ReadFromBuffer(char* output, size_t* msg_size);
void PrintBuffer();

#endif // CIRCULAR_BUFFER_H