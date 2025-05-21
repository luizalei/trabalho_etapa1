#include <windows.h>
#include <stdio.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include "circular_buffer.h"
#include <process.h>
#include <conio.h>
#include <sstream>
#include <time.h>
#include <chrono>
#include <thread>


#define BUFFER_SIZE 200
#define _CRT_SECURE_NO_WARNINGS
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;
DWORD WINAPI CLpThread(LPVOID);

// Struct da mensagem de sinalização
typedef struct {
    int32_t nseq;
    char tipo[3];       // "00"
    int8_t diag;
    int16_t remota;
    char id[9];
    int8_t estado;
    char timestamp[13];
} mensagem_ferrovia;

// Struct da mensagem de hotbox
typedef struct {
    int32_t nseq;
    char tipo[3];       // "99"
    char id[9];
    int8_t estado;
    char timestamp[13];
} mensagem_hotbox;

// Função utilitária para timestamp
void gerar_timestamp(char* timestamp) {
    SYSTEMTIME time;
    GetLocalTime(&time);
    sprintf_s(timestamp, 13, "%02d:%02d:%02d:%03d",
        time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
}

// Formatação da mensagem de sinalização
void formatar_mensagem_ferrovia(char* buffer, size_t buffer_size, const mensagem_ferrovia* msg) {
    sprintf_s(buffer, buffer_size, "%07d;%s;%d;%03d;%s;%d;%s",
        msg->nseq,
        msg->tipo,
        msg->diag,
        msg->remota,
        msg->id,
        msg->estado,
        msg->timestamp);
}

// Formatação da mensagem de roda quente
void formatar_mensagem_hotbox(char* buffer, size_t buffer_size, const mensagem_hotbox* msg) {
    sprintf_s(buffer, buffer_size, "%07d;%s;%s;%d;%s",
        msg->nseq,
        msg->tipo,
        msg->id,
        msg->estado,
        msg->timestamp);
}

int gcounter_ferrovia = 0;
int gcounter_hotbox = 0;

DWORD WINAPI CLpThread(LPVOID) {
    auto ultimoHotbox = std::chrono::steady_clock::now();

    while (true) {
        // --- Geração da mensagem de sinalização ferroviária ---
        mensagem_ferrovia msg_f;
        char buffer_f[100];

        msg_f.nseq = ++gcounter_ferrovia;
        strcpy_s(msg_f.tipo, sizeof(msg_f.tipo), "00");
        msg_f.diag = rand() % 2;
        msg_f.remota = rand() % 1000;

        // ID de sensor
        int numero = 1 + (rand() % 100);
        sprintf_s(msg_f.id, sizeof(msg_f.id), "SIN-%04d", numero);
        msg_f.estado = (rand() % 2) + 1;  // 1 ou 2
        gerar_timestamp(msg_f.timestamp);

        // Se falha de hardware (diag == 1)
        if (msg_f.diag == 1) {
            strcpy_s(msg_f.id, sizeof(msg_f.id), "XXXXXXXX");
            msg_f.estado = 0;
        }

        formatar_mensagem_ferrovia(buffer_f, sizeof(buffer_f), &msg_f);
        WriteToBuffer(buffer_f, 40);

        // --- Geração da mensagem de roda quente (a cada 500 ms) ---
        auto agora = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(agora - ultimoHotbox).count() >= 500) {
            mensagem_hotbox msg_h;
            char buffer_h[100];

            msg_h.nseq = ++gcounter_hotbox;
            strcpy_s(msg_h.tipo, sizeof(msg_h.tipo), "99");

            int idnum = 1 + (rand() % 100);
            sprintf_s(msg_h.id, sizeof(msg_h.id), "HWD-%04d", idnum);
            msg_h.estado = rand() % 2;  // 0 ou 1
            gerar_timestamp(msg_h.timestamp);

            formatar_mensagem_hotbox(buffer_h, sizeof(buffer_h), &msg_h);
            WriteToBuffer(buffer_h, 34);

            ultimoHotbox = agora;
        }

        // Espera aleatória entre 100 e 2000 ms para próxima sinalização
        int delay = (rand() % 1901) + 100;
        Sleep(delay);
    }

    return 0;
}

int main() {
    srand((unsigned int)time(NULL));  // Semente para rand
    InitializeBuffer();

    HANDLE hThread;
    DWORD dwThreadId;

    hThread = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION)CLpThread,
        NULL,
        0,
        (CAST_LPDWORD)&dwThreadId
    );

    if (hThread) {
        printf("Thread CLP unificada criada com ID=0x%x\n", dwThreadId);
    }

    // Exibe buffer a cada segundo até o usuário pressionar tecla
    while (!_kbhit()) {
        PrintBuffer();
        Sleep(1000);
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    DestroyBuffer();

    printf("\nPressione qualquer tecla para sair...\n");
    _getch();

    return 0;
}


