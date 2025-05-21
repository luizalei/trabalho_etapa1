#include <windows.h>
#include <stdio.h>
#define HAVE_STRUCT_TIMESPEC 
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include "circular_buffer.h"
#include <process.h>
#include <conio.h>
#include <sstream>

#define HAVE_STRUCT_TIMESPEC
#define _CRT_SECURE_NO_WARNINGS
#define BUFFER_SIZE 200  // Tamanho fixo da lista circular
#define _CHECKERROR 1
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;
DWORD WINAPI CLpThread(LPVOID);

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

//############## FUNÇÃO DE SIMULAÇÃO DO CLP ###############
DWORD WINAPI CLpThread(LPVOID) {

    do {
        //ADICIONAR CHECAGEM DE BUFFER CHEIO PÁRA BLOQUEAR A THREAD

		mensagem_ferrovia msg;
		char buffer[100]; // Buffer para armazenar a mensagem formatada
		
        // Preenche a mensagem com dados simulados
		msg.nseq = ++gcounter_ferrovia; // Incrementa o número sequencial
		strcpy_s(msg.tipo, sizeof(msg.tipo), "00");
		msg.diag = rand() % 2; // Diagnóstico
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

        //Mensagem recebida de um dos 100 sensores aleatoriamente
        int numero = 1 + (rand() % 100); // Gera número entre 1 e 100
        char sensor[9];
        sprintf_s(sensor, sizeof(sensor), "Sin-%04d", numero);
        strcpy_s(msg.id, sizeof(msg.id), sensor);

		msg.estado = rand()%2; // Estado 0 ou 1 aleatoriamente
		gerar_timestamp(msg.timestamp); // Gera o timestamp
		
        // Formata a mensagem completa
		formatar_mensagem(buffer, sizeof(buffer), &msg);
		
        // Escreve no buffer circular
		WriteToBuffer(buffer, 40);
        Sleep(1000);
		
        
        if (msg.diag == 1) { //Caso haja falha na remota
            strcpy_s(msg.id, sizeof(msg.id), "XXXXXXXX");
            msg.estado= 0;
        }
    }while (1);

    return 0;
}

int main() {
    InitializeBuffer();

    HANDLE hThread;
    DWORD dwThreadId;

    // Cria a thread CLp que escreve no buffer
    hThread = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION)CLpThread,
        NULL,
        0,
        (CAST_LPDWORD)&dwThreadId
    );

    if (hThread) {
        printf("Thread CLp criada com ID=0x%x\n", dwThreadId);
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

    printf("\nPressione qualquer tecla para sair...\n");
    _getch();

    return 0;
}

