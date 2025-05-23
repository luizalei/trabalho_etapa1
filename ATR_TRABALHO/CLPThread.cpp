#include <windows.h>
#include <stdio.h>
#define HAVE_STRUCT_TIMESPEC // tive que colocar isso devido a um problema que estava tendo sobre a biblioteca Pthread e o Vscode
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include "circular_buffer.h"
#include <process.h>
#include <conio.h>
#include <sstream>

#define __WIN32_WINNT 0X0500
#define HAVE_STRUCT_TIMESPEC
#define _CRT_SECURE_NO_WARNINGS
#define BUFFER_SIZE 200  // Tamanho fixo da lista circular
#define _CHECKERROR 1
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

HANDLE hEvent_ferrovia; // Handle para o evento de timeout da ferrovia
HANDLE WINAPI CreateTimerQueue(VOID);
DWORD WINAPI CLPThread(LPVOID);

//######### STRUCT MENSAGEM FERROVIA ##########
typedef struct {
    int32_t nseq;       // Número sequencial (1-9999999)
    char tipo[3];       // Sempre "00"
    int8_t diag;        // Diagnóstico (0-9)
    int16_t remota;     // Remota (000-999)
    char id[9];         // ID do sensor (8 chars + null terminator)
    int8_t estado;      // Estado (1 ou 2)
    char timestamp[13]; // HH:MM:SS:MS (12 chars + null terminator)
} mensagem_ferrovia;

// Função para gerar timestamp no formato HH:MM:SS:MS
void gerar_timestamp(char* timestamp) {
    SYSTEMTIME time;
    GetLocalTime(&time);
    // Usando sprintf_s com tamanho do buffer (13 para HH:MM:SS:MS)
    sprintf_s(timestamp, 13, "%02d:%02d:%02d:%03d",
        time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
}

// Função para formatar a mensagem completa
void formatar_mensagem(char* buffer, size_t buffer_size, const mensagem_ferrovia* msg) {
    // Assumindo que buffer_size é o tamanho total do buffer
    sprintf_s(buffer, buffer_size, "%07d;%s;%d;%03d;%s;%d;%s",
        msg->nseq,
        msg->tipo,
        msg->diag,
        msg->remota,
        msg->id,
        msg->estado,
        msg->timestamp);
}

int gcounter_ferrovia = 0; //contador para mensagem de ferrovia

void cria_msg_ferrovia() {
    


}

//############## FUNÇÃO DE SIMULAÇÃO DO CLP ###############
DWORD WINAPI CLPThread(LPVOID) {

    do {
        HANDLE hTimerQueue;
        HANDLE hEvent_ferrovia;
        BOOL status_queue;
        hTimerQueue = CreateTimerQueue();
        if (hTimerQueue == NULL) {
            printf("Falha em CreateTimerQueue! Codigo =%d)\n", GetLastError());
            return 0;
        }
        // Enfileira primeiro temporizador com sua fun��o callback
        status_queue = CreateTimerQueueTimer(&hTimerID1, hTimerQueue, (WAITORTIMERCALLBACK)Pid,
            NULL, 7000, 5000, WT_EXECUTEDEFAULT);
        //NULL, 1000, 1000, WT_EXECUTEDEFAULT);
        if (!status) {
            printf("Erro em CreateTimerQueueTimer [1]! Codigo = %d)\n", GetLastError());
            return 0;
        }

        int t_ferrovia = 100 + (rand() % 1899); //Tempo aléatorio para envio de mensagens do tipo ferrovia 
        printf("tempo msg ferrovia %d \n", t_ferrovia);
        
        status = WaitForSingleObject(hEvent_ferrovia, t_ferrovia);
        printf("STATUS: %d \n", status);
        Sleep(1000);

        if (status == WAIT_TIMEOUT) {
            mensagem_ferrovia msg;
            char buffer[100]; // Buffer para armazenar a mensagem formatada

            // Preenche a mensagem com dados simulados
            msg.nseq = ++gcounter_ferrovia; // Incrementa o número sequencial
            strcpy_s(msg.tipo, sizeof(msg.tipo), "00");

            //Mensagem recebida de um dos 100 sensores aleatoriamente
            int numero = 1 + (rand() % 100); // Gera número entre 1 e 100
            char sensor[9];
            sprintf_s(sensor, sizeof(sensor), "Sin-%04d", numero);
            strcpy_s(msg.id, sizeof(msg.id), sensor);

            // Diagnóstico
            msg.diag = rand() % 2; 
            if (msg.diag == 1) { //Caso haja falha na remota
                strcpy_s(msg.id, sizeof(msg.id), "XXXXXXXX");
                msg.estado = 0;
            }

            //Remota
            int remota = rand() % 1000; // Gera um número entre 0 e 999
            std::stringstream ss;
            if (remota < 10) {
                ss << "00" << remota;       // Ex: 5 → 005
            }
            else if (remota < 100) {
                ss << "0" << remota;        // Ex: 58 → 058
            }
            else {
                ss << remota;               // Ex: 123 → 123
            }
            msg.remota = remota;

            msg.estado = rand() % 2; // Estado 0 ou 1 aleatoriamente
            gerar_timestamp(msg.timestamp); // Gera o timestamp

            // Formata a mensagem completa
            formatar_mensagem(buffer, sizeof(buffer), &msg);

            // Escreve no buffer circular
            WriteToBuffer(buffer, 40);
        }
        //ADICIONAR CHECAGEM DE BUFFER CHEIO PARA BLOQUEAR A THREAD

        
    }while (1);

    return 0;
}

int main() {
    InitializeBuffer();

    HANDLE hThread;
    DWORD dwThreadId;
    hEvent_ferrovia = CreateEvent(NULL, TRUE, FALSE, L"EvTimeOutFerrovia");

    // Cria a thread CLP que escreve no buffer
    hThread = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION)CLPThread,
        NULL,
        0,
        (CAST_LPDWORD)&dwThreadId
    );

    if (hThread) {
        printf("Thread CLP criada com ID=0x%x\n", dwThreadId);
    }
    else {
        //CheckForError(FALSE);
    }

    // Loop principal que lê e exibe o buffer periodicamente
    while (!_kbhit()) {
        PrintBuffer();
        Sleep(1000); // Verifica o buffer a cada segundo
    }

    // Limpeza
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    DestroyBuffer();
    DeleteTimerQueueTimerEX();

    printf("\nPressione qualquer tecla para sair...\n");
    _getch();

    return 0;
}

