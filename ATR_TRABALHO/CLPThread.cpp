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
HANDLE hEvent_roda; // Handle para o evento de timeout da roda
HANDLE WINAPI CreateTimerQueue(VOID);
HANDLE hBufferRodaCheio;  // Evento para sinalizar espaço no buffer
HANDLE hMutexBufferRoda; // Mutex para proteger o buffer de roda quente
HANDLE hMutexBufferFerrovia; // Mutex para proteger o buffer de ferrovia

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

//######### STRUCT MENSAGEM RODA ##########
typedef struct {
    int32_t nseq;       // Número sequencial (1-9999999)
    char tipo[3];       // Sempre "00"
    int8_t diag;        // Diagnóstico (0-9)
    int16_t remota;     // Remota (000-999)
    char id[9];         // ID do sensor (8 chars + null terminator)
    int8_t estado;      // Estado (1 ou 2)
    char timestamp[13]; // HH:MM:SS:MS (12 chars + null terminator)
} mensagem_roda;

//########## Função para gerar timestamp no formato HH:MM:SS:MS ################
void gerar_timestamp(char* timestamp) {
    SYSTEMTIME time;
    GetLocalTime(&time);
    // Usando sprintf_s com tamanho do buffer (13 para HH:MM:SS:MS)
    sprintf_s(timestamp, 13, "%02d:%02d:%02d:%03d",
        time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
}

//############# Função para formatar a mensagem ferrovia ###################
void formatar_msg_ferrovia(char* buffer, size_t buffer_size, const mensagem_ferrovia* msg) {
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

//############# Função para formatar a mensagem de roda quente ###################
void formatar_msg_roda(char* buffer, size_t buffer_size, const mensagem_roda* msg) {
    // Formata a mensagem no padrão especificado: NNNNNNN;NN;AAAAAAAA;N;HH:MM:SS:MS
    // Total: 7 + 1 + 2 + 1 + 8 + 1 + 1 + 1 + 12 = 34 caracteres
    sprintf_s(buffer, buffer_size, "%07d;%s;%s;%d;%s",
        msg->nseq,      // 7 dígitos (NSEQ)
        msg->tipo,      // 2 caracteres (TIPO)
        msg->id,        // 8 caracteres (ID)
        msg->estado,    // 1 dígito (ESTADO)
        msg->timestamp  // 12 caracteres (TIMESTAMP)
    );
}

int gcounter_ferrovia = 0; //contador para mensagem de ferrovia
int gcounter_roda = 0; //contador para mensagem de roda


void cria_msg_ferrovia() {
    mensagem_ferrovia msg;
    char buffer[MAX_MSG_LENGTH]; // Buffer para armazenar a mensagem formatada

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
    formatar_msg_ferrovia(buffer, sizeof(buffer), &msg);

    // Escreve no buffer circular
    WriteToFerroviaBuffer(buffer);
}

void cria_msg_roda() {
    mensagem_roda msg;
    char buffer[SMALL_MSG_LENGTH]; // Buffer para armazenar a mensagem formatada

    // Preenche a mensagem com dados simulados
    msg.nseq = ++gcounter_roda; // Incrementa o número sequencial
    strcpy_s(msg.tipo, sizeof(msg.tipo), "99");

    //Mensagem recebida de um dos 100 sensores aleatoriamente
    int numero = 1 + (rand() % 100); // Gera número entre 1 e 100
    char sensor[9];
    sprintf_s(sensor, sizeof(sensor), "Sin-%04d", numero);
    strcpy_s(msg.id, sizeof(msg.id), sensor);

    //Estado
    msg.estado = rand() % 2; // Estado 0 ou 1 aleatoriamente

    gerar_timestamp(msg.timestamp); // Gera o timestamp

    // Formata a mensagem completa
    formatar_msg_roda(buffer, sizeof(buffer), &msg);

    // Escreve no buffer circular
    WriteToRodaBuffer(buffer);
}

//############## FUNÇÃO DE SIMULAÇÃO DO CLP ###############
DWORD WINAPI CLPThread(LPVOID) {
    HANDLE hTimerQueue;
    HANDLE hEvent_ferrovia;
    HANDLE hEvent_roda;
    BOOL status_queue;

    //Cria fila de temporizadores
    hTimerQueue = CreateTimerQueue();
    if (hTimerQueue == NULL) {
        printf("Falha em CreateTimerQueue! Codigo =%d)\n", GetLastError());
        return 0;
    }

    // Enfileira o temporizador da roda quente, fora do DO WHILE porque tem temporização fixa e posso usar a temporização periódica
    status_queue = CreateTimerQueueTimer(&hEvent_roda, hTimerQueue, (WAITORTIMERCALLBACK)cria_msg_roda,
        NULL, 500, 500, WT_EXECUTEDEFAULT);
    if (!status_queue) {
        printf("Erro em CreateTimerQueueTimer [2]! Codigo = %d)\n", GetLastError());
        return 0;
    }

    do {
        // Verifica buffer roda
        //Conquista MUTEX
        WaitForSingleObject(hMutexBufferRoda, INFINITE);
        BOOL rodaCheia = rodaBuffer.isFull;
		ReleaseMutex(hMutexBufferRoda); //Libera MUTEX

        if (rodaCheia) {
            WaitForSingleObject(rodaBuffer.hEventSpaceAvailable, INFINITE);
        }

        // Verifica buffer ferrovia
        WaitForSingleObject(hMutexBufferFerrovia, INFINITE);
        BOOL ferroviaCheia = ferroviaBuffer.isFull;
        ReleaseMutex(hMutexBufferFerrovia); //Libera MUTEX

        if (ferroviaCheia) {
            WaitForSingleObject(ferroviaBuffer.hEventSpaceAvailable, INFINITE);
        }

        // Verifica se os buffers estão cheios antes de criar nova mensagem, se sim, bloquea a thread até que esvaziem


        int tempo_ferrovia = 100 + (rand() % 1901); //tempo aleatório entre 100 e 2000ms
        Sleep(tempo_ferrovia); //Temporizador temporário para a entrega da etapa 1
		cria_msg_ferrovia(); // Chama a função de criação da mensagem de ferrovia
        
   
    }while (1);

    return 0;
}

int main() {
    InitializeBuffers();

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

    hMutexBufferRoda = CreateMutex(NULL, FALSE, L"BufferRoda");
    hMutexBufferFerrovia = CreateMutex(NULL, FALSE, L"BufferFerrovia");

    // Loop principal que lê e exibe o buffer periodicamente
    while (!_kbhit()) {
        PrintBuffers();
        Sleep(1000); // Verifica o buffer a cada segundo
    }

    // Limpeza
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    DestroyBuffers();
    CloseHandle(hMutexBufferRoda);
    CloseHandle(hMutexBufferFerrovia);

    printf("\nPressione qualquer tecla para sair...\n");
    _getch();

    return 0;
}

