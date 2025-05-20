#include "circular_buffer.h"
#include <string.h>

CircularBuffer circularBuffer;

void InitializeBuffer() {
    circularBuffer.head = 0;
    circularBuffer.tail = 0;
    circularBuffer.count = 0;
    InitializeCriticalSection(&circularBuffer.cs);
}

void DestroyBuffer() {
    DeleteCriticalSection(&circularBuffer.cs);
}

void WriteToBuffer(const char* value, size_t msg_size) {
    EnterCriticalSection(&circularBuffer.cs);

    if (circularBuffer.count < BUFFER_SIZE) {
        if (msg_size == 34 || msg_size == 40) {
            size_t buffer_size = (msg_size == 34) ? SMALL_MSG_LENGTH : MAX_MSG_LENGTH;
            size_t copy_size = (msg_size == 34) ? SMALL_MSG_LENGTH - 1 : MAX_MSG_LENGTH - 1;

            // Usando strncpy_s com verificação de tamanho
            errno_t err = strncpy_s(circularBuffer.data[circularBuffer.tail],
                buffer_size,
                value,
                copy_size);

            if (err == 0) {
                circularBuffer.tail = (circularBuffer.tail + 1) % BUFFER_SIZE;
                circularBuffer.count++;
            }
            else {
                printf("Erro ao copiar mensagem para o buffer: %d\n", err);
            }
        }
        else {
            printf("Tamanho de mensagem inválido: %zu\n", msg_size);
        }
    }
    else {
        printf("Buffer cheio - mensagem descartada: %s\n", value);
    }

    LeaveCriticalSection(&circularBuffer.cs);
}

int ReadFromBuffer(char* output, size_t* msg_size) {
    EnterCriticalSection(&circularBuffer.cs);

    int result = 0;
    if (circularBuffer.count > 0) {
        size_t len = strlen(circularBuffer.data[circularBuffer.head]);
        if (len == 34 || len == 40) {
            // Usando strncpy_s com verificação de tamanho
            errno_t err = strncpy_s(output,
                (len == 34) ? SMALL_MSG_LENGTH : MAX_MSG_LENGTH,
                circularBuffer.data[circularBuffer.head],
                len);

            if (err == 0) {
                *msg_size = len;
                circularBuffer.head = (circularBuffer.head + 1) % BUFFER_SIZE;
                circularBuffer.count--;
                result = 1;
            }
            else {
                printf("Erro ao ler mensagem do buffer: %d\n", err);
            }
        }
        else {
            printf("Mensagem corrompida no buffer - tamanho inválido: %zu\n", len);
        }
    }
    else {
        printf("Buffer vazio - nada para ler.\n");
    }

    LeaveCriticalSection(&circularBuffer.cs);
    return result;
}

void PrintBuffer() {
    EnterCriticalSection(&circularBuffer.cs);

    printf("Buffer [%d/%d]:\n", circularBuffer.count, BUFFER_SIZE);
    int index = circularBuffer.head;
    for (int i = 0; i < circularBuffer.count; i++) {
        printf("%s (%zu chars)\n", circularBuffer.data[index], strlen(circularBuffer.data[index]));
        index = (index + 1) % BUFFER_SIZE;
    }
    printf("\n");

    LeaveCriticalSection(&circularBuffer.cs);
}