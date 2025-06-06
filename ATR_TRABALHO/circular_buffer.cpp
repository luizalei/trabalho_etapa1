#include "circular_buffer.h"
#include <string.h>

static CircularBuffer circularBuffer;

void InitializeBuffer() {
    circularBuffer.head = 0;
    circularBuffer.tail = 0;
    circularBuffer.count = 0;
    InitializeCriticalSection(&circularBuffer.cs);

    circularBuffer.sem_vaga = CreateSemaphore(NULL, BUFFER_SIZE, BUFFER_SIZE, NULL);
    circularBuffer.sem_ocupado = CreateSemaphore(NULL, 0, BUFFER_SIZE, NULL);
}

void DestroyBuffer() {
    DeleteCriticalSection(&circularBuffer.cs);
    CloseHandle(circularBuffer.sem_vaga);
    CloseHandle(circularBuffer.sem_ocupado);
}

void WriteToBuffer(const char* value, size_t msg_size) {
    // Espera vaga disponível
    WaitForSingleObject(circularBuffer.sem_vaga, INFINITE);

    EnterCriticalSection(&circularBuffer.cs);

    if (msg_size == 34 || msg_size == 40) {
        size_t copy_size = (msg_size < MAX_MSG_LENGTH) ? msg_size : MAX_MSG_LENGTH - 1;

        errno_t err = strncpy_s(
            circularBuffer.data[circularBuffer.tail],
            MAX_MSG_LENGTH,
            value,
            copy_size
        );

        if (err != 0) {
            printf("[Buffer] Erro ao copiar mensagem: %d\n", err);
        }
        else {
            circularBuffer.data[circularBuffer.tail][copy_size] = '\0'; // Garante null-terminador
            circularBuffer.tail = (circularBuffer.tail + 1) % BUFFER_SIZE;
            circularBuffer.count++;
        }
    }
    else {
        printf("[Buffer] Tamanho de mensagem inválido: %zu\n", msg_size);
    }

    LeaveCriticalSection(&circularBuffer.cs);
    ReleaseSemaphore(circularBuffer.sem_ocupado, 1, NULL);
}

int ReadFromBuffer(char* output, size_t* msg_size) {
    // Espera até que haja mensagem
    WaitForSingleObject(circularBuffer.sem_ocupado, INFINITE);

    EnterCriticalSection(&circularBuffer.cs);

    int result = 0;
    size_t len = strlen(circularBuffer.data[circularBuffer.head]);

    if (len == 34 || len == 40) {
        errno_t err = strncpy_s(output, MAX_MSG_LENGTH, circularBuffer.data[circularBuffer.head], len);

        if (err == 0) {
            output[len] = '\0';
            *msg_size = len;
            circularBuffer.head = (circularBuffer.head + 1) % BUFFER_SIZE;
            circularBuffer.count--;
            result = 1;
        }
        else {
            printf("[Buffer] Erro ao ler mensagem: %d\n", err);
        }
    }
    else {
        printf("[Buffer] Mensagem corrompida no buffer (tamanho inválido: %zu)\n", len);
    }

    LeaveCriticalSection(&circularBuffer.cs);
    ReleaseSemaphore(circularBuffer.sem_vaga, 1, NULL);

    return result;
}

void PrintBuffer() {
    EnterCriticalSection(&circularBuffer.cs);

    printf("[Buffer] Conteúdo (%d mensagens):\n", circularBuffer.count);
    int index = circularBuffer.head;

    for (int i = 0; i < circularBuffer.count; i++) {
        printf("  %s\n", circularBuffer.data[index]);
        index = (index + 1) % BUFFER_SIZE;
    }

    LeaveCriticalSection(&circularBuffer.cs);
}
