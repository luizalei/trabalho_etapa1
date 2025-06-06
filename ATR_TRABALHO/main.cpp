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

//############ DEFINIÇÕES GLOBAIS ############
#define __WIN32_WINNT 0X0500
#define HAVE_STRUCT_TIMESPEC
#define _CRT_SECURE_NO_WARNINGS
#define BUFFER_SIZE 200  // Tamanho fixo da lista circular
#define _CHECKERROR 1
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

//############# HANDLES ##########
HANDLE hEvent_ferrovia; // Handle para o evento de timeout da ferrovia
HANDLE hEvent_roda; // Handle para o evento de timeout da roda
HANDLE WINAPI CreateTimerQueue(VOID);
HANDLE hBufferRodaCheio;  // Evento para sinalizar espaço no buffer

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

//########## FUNÇÃO PARA TIMESTAMP HH:MM:SS:MS ################
void gerar_timestamp(char* timestamp) {
    SYSTEMTIME time;
    GetLocalTime(&time);
    // Usando sprintf_s com tamanho do buffer (13 para HH:MM:SS:MS)
    sprintf_s(timestamp, 13, "%02d:%02d:%02d:%03d",
        time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
}

//############# FUNÇÃO FORMATA MSG FERROVIA ###################
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

//############# FUNÇÃO FORMATA MSG RODA QUENTE ###################
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

//############# CONTADORES GLOBAIS PARA MENSAGENS ################
int gcounter_ferrovia = 0; //contador para mensagem de ferrovia
int gcounter_roda = 0; //contador para mensagem de roda

//############# FUNÇÃO DE CRIAÇÃO DE MSG FERROVIA ################
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

//############# FUNÇÃO DE CRIAÇÃO DE MSG RODA QUENTE ################
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

//############## FUNÇÃO DA THREAD DE SIMULAÇÃO DO CLP ###############
DWORD WINAPI CLPThread(LPVOID) {
    // Inicializa os eventos e a fila de temporizadores
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
        WaitForSingleObject(hMutexBufferRoda, INFINITE);//Conquista MUTEX
        BOOL rodaCheia = rodaBuffer.isFull;
        ReleaseMutex(hMutexBufferRoda); //Libera MUTEX

        if (rodaCheia) {
            WaitForSingleObject(rodaBuffer.hEventSpaceAvailable, INFINITE);
        }

        // Verifica buffer ferrovia
        WaitForSingleObject(hMutexBufferFerrovia, INFINITE); //Conquista MUTEX
        BOOL ferroviaCheia = ferroviaBuffer.isFull;
        ReleaseMutex(hMutexBufferFerrovia); //Libera MUTEX

        if (ferroviaCheia) {
            WaitForSingleObject(ferroviaBuffer.hEventSpaceAvailable, INFINITE);
        }

        int tempo_ferrovia = 100 + (rand() % 1901); //tempo aleatório entre 100 e 2000ms
        //SERÁ SUBSTITUIDO NA ENTREGA DA ETAPA 2 PARA RETIRADA DA FUNÇÃO SLEEP()
        Sleep(tempo_ferrovia); //Temporizador temporário para a entrega da etapa 1
        cria_msg_ferrovia(); // Chama a função de criação da mensagem de ferrovia

    } while (1); //MUDAR PARA QUE A THREAD DE LEITURA DO TECLADO ENCERRE ESSA

    return 0;
}

//############## FUNÇÃO DA THREAD DE CAPTURA DE RODA QUENTE###############
DWORD WINAPI CapturaHotboxThread(LPVOID) {
    char mensagem[SMALL_MSG_LENGTH];

    printf("[Hotbox-Captura] Thread iniciada.\n");

    while (1) {
        if (ReadFromRodaBuffer(mensagem)) {
            // Confirma se é uma mensagem tipo 99
            if (mensagem[8] == '9' && mensagem[9] == '9') {
                printf("[Hotbox] %s\n", mensagem);
            }
        }
        else {
            Sleep(100); //reduz o consumo de CPU e ainda mantém a verificação do buffer com certa frequente (10 vezes por segundo)
        }
    }

    return 0;
}

//############## FUNÇÃO DA THREAD DE CAPTURA DE SINALIZAÇÃO FERROVIÁRIA ###############
DWORD WINAPI CapturaSinalizacaoThread(LPVOID) {
    char mensagem[MAX_MSG_LENGTH];

    printf("[Ferrovia-Captura] Thread iniciada.\n");

    while (1) {
        if (ReadFromFerroviaBuffer(mensagem)) {
            // Verifica se é mensagem tipo 00 (sinalização ferroviária)
            if (mensagem[8] == '0' && mensagem[9] == '0') {
                printf("[Ferrovia] %s\n", mensagem);
            }
        }
        else {
            Sleep(100); // Evita uso intenso da CPU
        }
    }

    return 0;
}

//############# FUNÇÃO MAIN DO SISTEMA ################
int main() {
    InitializeBuffers();

    HANDLE hCLPThread;
    HANDLE hCapturaHotboxThread;
    DWORD dwThreadId;
    hEvent_ferrovia = CreateEvent(NULL, TRUE, FALSE, L"EvTimeOutFerrovia");


    // Cria a thread CLP que escreve no buffer
    hCLPThread = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION)CLPThread,
        NULL,
        0,
        (CAST_LPDWORD)&dwThreadId
    );

    if (hCLPThread) {
        printf("Thread CLP criada com ID=0x%x\n", dwThreadId);
    }
    // CRia a thread de Captura de Dados dos HotBoxes
    hCapturaHotboxThread = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION)CapturaHotboxThread,
        NULL,
        0,
        (CAST_LPDWORD)&dwThreadId
    );
    if (hCapturaHotboxThread) {
        printf("Thread CapturaHotbox criada com ID=0x%x\n", dwThreadId);
    }

    // Cria a thread de Captura de Dados da Sinalização Ferroviária
    HANDLE hCapturaSinalizacaoThread;
    hCapturaSinalizacaoThread = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION)CapturaSinalizacaoThread,
        NULL,
        0,
        (CAST_LPDWORD)&dwThreadId
    );

    if (hCapturaSinalizacaoThread) {
        printf("Thread CapturaSinalizacao criada com ID=0x%x\n", dwThreadId);
    }


    // === CRIAÇÃO DO PROCESSO VISUALIZA_HOTBOXES.EXE COM NOVO CONSOLE ===
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    WCHAR currentDir[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, currentDir);
    wprintf(L"[DEBUG] Diretório atual: %s\n", currentDir);

    // Criação de processo separado com novo console
    if (CreateProcess(
        L"C:\\Users\\laert\\source\\repos\\trabalho_etapa1\\x64\\Debug\\VisualizaHotboxes.exe", // Nome do executável do processo filho
        NULL,                      // Argumentos da linha de comando
        NULL,                      // Atributos de segurança do processo
        NULL,                      // Atributos de segurança da thread
        FALSE,                     // Herança de handles
        CREATE_NEW_CONSOLE,       // Cria nova janela de console
        NULL,                      // Ambiente padrão
        NULL,                      // Diretório padrão
        &si,                       // Informações de inicialização
        &pi                        // Informações sobre o processo criado
    )) {
        printf("Processo visualiza_hotboxes.exe criado com sucesso!\n");
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        printf("Erro ao criar processo visualiza_hotboxes.exe. Código do erro: %lu\n", GetLastError());
    }
    // Loop principal que lê e exibe o buffer periodicamente
    while (!_kbhit()) {
        PrintBuffers();
        Sleep(1000); // Verifica o buffer a cada segundo
    }

    // Limpeza
    WaitForSingleObject(hCLPThread, INFINITE);
    WaitForSingleObject(hCapturaHotboxThread, INFINITE);
    WaitForSingleObject(hCapturaSinalizacaoThread, INFINITE);

    CloseHandle(hCLPThread);
    CloseHandle(hCapturaHotboxThread);
    CloseHandle(hCapturaSinalizacaoThread);

    DestroyBuffers();
    CloseHandle(hMutexBufferRoda);
    CloseHandle(hMutexBufferFerrovia);

    printf("\nPressione qualquer tecla para sair...\n");
    _getch();

    return 0;
}