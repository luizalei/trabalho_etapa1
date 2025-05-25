#include "circular_buffer.h"
#include <string.h>

CircularBuffer ferroviaBuffer;
CircularBuffer rodaBuffer;

void InitializeBuffers() {
    // Initialize ferrovia buffer
    ferroviaBuffer.head = 0;
    ferroviaBuffer.tail = 0;
    ferroviaBuffer.count = 0;
    ferroviaBuffer.isFull = FALSE;
    InitializeCriticalSection(&ferroviaBuffer.cs);
    ferroviaBuffer.hEventSpaceAvailable = CreateEvent(NULL, TRUE, TRUE, NULL);

    // Initialize roda buffer
    rodaBuffer.head = 0;
    rodaBuffer.tail = 0;
    rodaBuffer.count = 0;
    rodaBuffer.isFull = FALSE;
    InitializeCriticalSection(&rodaBuffer.cs);
    rodaBuffer.hEventSpaceAvailable = CreateEvent(NULL, TRUE, TRUE, NULL);
}

void DestroyBuffers() {
    DeleteCriticalSection(&ferroviaBuffer.cs);
    DeleteCriticalSection(&rodaBuffer.cs);
    CloseHandle(ferroviaBuffer.hEventSpaceAvailable);
    CloseHandle(rodaBuffer.hEventSpaceAvailable);
}

void WriteToFerroviaBuffer(const char* value) {
    EnterCriticalSection(&ferroviaBuffer.cs);

    // Se o buffer está cheio, não escreve e espera
    while (ferroviaBuffer.count >= BUFFER_SIZE) {
        ferroviaBuffer.isFull = TRUE;
        ResetEvent(ferroviaBuffer.hEventSpaceAvailable);
        LeaveCriticalSection(&ferroviaBuffer.cs);

        // Espera por espaço disponível
        WaitForSingleObject(ferroviaBuffer.hEventSpaceAvailable, INFINITE);

        EnterCriticalSection(&ferroviaBuffer.cs);
    }

    errno_t err = strncpy_s(ferroviaBuffer.data[ferroviaBuffer.tail],
        MAX_MSG_LENGTH,
        value,
        MAX_MSG_LENGTH - 1);

    if (err == 0) {
        ferroviaBuffer.tail = (ferroviaBuffer.tail + 1) % BUFFER_SIZE;
        ferroviaBuffer.count++;
        ferroviaBuffer.isFull = (ferroviaBuffer.count >= BUFFER_SIZE);
    }
    else {
        printf("Erro ao copiar mensagem para o buffer ferrovia: %d\n", err);
    }

    LeaveCriticalSection(&ferroviaBuffer.cs);
}

void WriteToRodaBuffer(const char* value) {
    EnterCriticalSection(&rodaBuffer.cs);

    while (rodaBuffer.count >= BUFFER_SIZE) {
        rodaBuffer.isFull = TRUE;
        ResetEvent(rodaBuffer.hEventSpaceAvailable); // Garante que o evento não está sinalizado
        LeaveCriticalSection(&rodaBuffer.cs);

        // Espera por espaço disponível
        WaitForSingleObject(rodaBuffer.hEventSpaceAvailable, INFINITE);

        EnterCriticalSection(&rodaBuffer.cs);
    }

    errno_t err = strncpy_s(rodaBuffer.data[rodaBuffer.tail],
        SMALL_MSG_LENGTH,
        value,
        SMALL_MSG_LENGTH - 1);

    if (err == 0) {
        rodaBuffer.tail = (rodaBuffer.tail + 1) % BUFFER_SIZE;
        rodaBuffer.count++;
        rodaBuffer.isFull = (rodaBuffer.count >= BUFFER_SIZE);
    }
    else {
        printf("Erro ao copiar mensagem para o buffer roda: %d\n", err);
    }

    LeaveCriticalSection(&rodaBuffer.cs);
}

int ReadFromFerroviaBuffer(char* output) {
    EnterCriticalSection(&ferroviaBuffer.cs);

    int result = 0;
    if (ferroviaBuffer.count > 0) {
        errno_t err = strncpy_s(output,
            MAX_MSG_LENGTH,
            ferroviaBuffer.data[ferroviaBuffer.head],
            MAX_MSG_LENGTH - 1);

        if (err == 0) {
            ferroviaBuffer.head = (ferroviaBuffer.head + 1) % BUFFER_SIZE;
            ferroviaBuffer.count--;

            // Se estava cheio e agora tem espaço, sinaliza
            if (ferroviaBuffer.isFull && ferroviaBuffer.count < BUFFER_SIZE) {
                ferroviaBuffer.isFull = FALSE;
                SetEvent(ferroviaBuffer.hEventSpaceAvailable);
            }

            result = 1;
        }
        else {
            printf("Erro ao ler mensagem do buffer ferrovia: %d\n", err);
        }
    }
    else {
        printf("Buffer ferrovia vazio - nada para ler.\n");
    }

    LeaveCriticalSection(&ferroviaBuffer.cs);
    return result;
}

int ReadFromRodaBuffer(char* output) {
    EnterCriticalSection(&rodaBuffer.cs);

    int result = 0;
    if (rodaBuffer.count > 0) {
        errno_t err = strncpy_s(output,
            SMALL_MSG_LENGTH,
            rodaBuffer.data[rodaBuffer.head],
            SMALL_MSG_LENGTH - 1);

        if (err == 0) {
            rodaBuffer.head = (rodaBuffer.head + 1) % BUFFER_SIZE;
            rodaBuffer.count--;

            // Se estava cheio e agora tem espaço, sinaliza
            if (rodaBuffer.isFull && rodaBuffer.count < BUFFER_SIZE) {
                rodaBuffer.isFull = FALSE;
                SetEvent(rodaBuffer.hEventSpaceAvailable);
            }

            result = 1;
        }
        else {
            printf("Erro ao ler mensagem do buffer roda: %d\n", err);
        }
    }
    else {
        printf("Buffer roda vazio - nada para ler.\n");
    }

    LeaveCriticalSection(&rodaBuffer.cs);
    return result;
}

void PrintBuffers() {
    EnterCriticalSection(&ferroviaBuffer.cs);
    EnterCriticalSection(&rodaBuffer.cs);

    printf("=== Buffers ===\n");
    printf("Ferrovia [%d/%d] %s:\n", ferroviaBuffer.count, BUFFER_SIZE,
        ferroviaBuffer.isFull ? "(CHEIO)" : "");
    int index = ferroviaBuffer.head;
    for (int i = 0; i < ferroviaBuffer.count; i++) {
        printf("%s\n", ferroviaBuffer.data[index]);
        index = (index + 1) % BUFFER_SIZE;
    }

    printf("\nRoda [%d/%d] %s:\n", rodaBuffer.count, BUFFER_SIZE,
        rodaBuffer.isFull ? "(CHEIO)" : "");
    index = rodaBuffer.head;
    for (int i = 0; i < rodaBuffer.count; i++) {
        printf("%s\n", rodaBuffer.data[index]);
        index = (index + 1) % BUFFER_SIZE;
    }
    printf("\n");

    LeaveCriticalSection(&rodaBuffer.cs);
    LeaveCriticalSection(&ferroviaBuffer.cs);
}