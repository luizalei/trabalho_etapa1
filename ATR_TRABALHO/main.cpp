﻿#include <windows.h>
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
#include <wchar.h>

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
//HANDLE WINAPI CreateTimerQueue(VOID);
HANDLE hBufferRodaCheio;  // Evento para sinalizar espaço no buffer
//handles para a tarefa de leitura do teclado
HANDLE evCLPFerrovia_PauseResume, evCLPHotbox_PauseResume, evFERROVIA_PauseResume, evHOTBOX_PauseResume, evTemporização;
HANDLE evVISUFERROVIA_PauseResume, evVISUHOTBOX_PauseResume;
HANDLE evCLP_Exit, evFERROVIA_Exit, evHOTBOX_Exit;
HANDLE evVISUFERROVIA_Exit, evVISUHOTBOX_Exit;
HANDLE evEncerraThreads=NULL;
DWORD WINAPI hCLPThreadFerrovia(LPVOID);
DWORD WINAPI hCLPThreadRoda(LPVOID);

//######### STRUCT MENSAGEM FERROVIA ##########
typedef struct {
    int32_t nseq;       // Número sequencial (1-9999999)
    char tipo[3];       // Sempre "00"
    int8_t diag;        // Diagnóstico (0-9)
    int16_t remota;     // Remota (000-999)
    char id[9];         // ID do sensor
    int8_t estado;      // Estado (1 ou 2)
    char timestamp[13]; // HH:MM:SS:MS 
} mensagem_ferrovia;

//######### STRUCT MENSAGEM RODA ##########
typedef struct {
    int32_t nseq;       // Número sequencial (1-9999999)
    char tipo[3];       // Sempre "00"
    int8_t diag;        // Diagnóstico (0-9)
    int16_t remota;     // Remota (000-999)
    char id[9];         // ID do sensor 
    int8_t estado;      // Estado (1 ou 2)
    char timestamp[13]; // HH:MM:SS:MS 
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
    sprintf_s(buffer, buffer_size, "%07d;%s;%s;%d;%s",
        msg->nseq,      
        msg->tipo,      
        msg->id,        
        msg->estado,    
        msg->timestamp  
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
    printf("\033[32mMensagem Ferrovia criada: %s\033[0m\n", buffer);
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
    printf("\033[32mMensagem Hotbox criada: %s\033[0m\n", buffer);
}


//############# THREAD CRIA MENSAGENS DE FERROVIA CLP #############
DWORD WINAPI CLPMsgFerrovia(LPVOID) {
    BOOL pausado = FALSE;
    HANDLE eventos[2] = {evCLP_Exit, evCLPFerrovia_PauseResume };

    while (1) {
        // Verifica buffer ferrovia
        WaitForSingleObject(hMutexBufferFerrovia, INFINITE); //Conquista MUTEX
        BOOL ferroviaCheia = ferroviaBuffer.isFull;
        ReleaseMutex(hMutexBufferFerrovia); //Libera MUTEX

        if (ferroviaCheia) {
            WaitForSingleObject(ferroviaBuffer.hEventSpaceAvailable, INFINITE);
        }

        // Verifica eventos sem bloquear 
        DWORD ret = WaitForMultipleObjects(2, eventos, FALSE, 0);

        switch (ret) {
        case WAIT_OBJECT_0: // evCLP_Exit
            return 0;

        case WAIT_OBJECT_0 + 1: // evCLPFerrovia_PauseResume
            pausado = !pausado;
            printf("Thread Ferrovia %s\n", pausado ? "PAUSADA" : "RETOMADA");
            ResetEvent(evCLPFerrovia_PauseResume);
            break;

        case WAIT_TIMEOUT:

            break;

        default:
            printf("Erro: %d\n", GetLastError());
            return 1;
        }

        if (!pausado) { //Se a thread estiver permitida de rodar

            int tempo_ferrovia = 100 + (rand() % 1901); // 100-2000ms

            WaitForSingleObject(evTemporização, tempo_ferrovia); // evento que nunca será setado apenas para bloquear a thread 

            //Sleep(tempo_ferrovia);
            cria_msg_ferrovia();
        }
        else {
            // Se pausado, verifica eventos mais frequentemente
			WaitForSingleObject(evTemporização, 100); // evento que nunca será setado apenas para bloquear a thread
            //Sleep(100);
        }
    }
    return 0;
}

//############# THREAD CRIA MENSAGENS DE RODA QUENTE CLP #############
DWORD WINAPI CLPMsgRodaQuente(LPVOID) {
    BOOL pausado = FALSE;
    HANDLE eventos[2] = {evCLP_Exit, evCLPHotbox_PauseResume};

    while (1) {
        // Verifica buffer roda
        WaitForSingleObject(hMutexBufferRoda, INFINITE);//Conquista MUTEX
        BOOL rodaCheia = rodaBuffer.isFull;
        ReleaseMutex(hMutexBufferRoda); //Libera MUTEX

        if (rodaCheia) {
            WaitForSingleObject(rodaBuffer.hEventSpaceAvailable, INFINITE);
        }


        // Verifica eventos sem bloquear 
        DWORD ret = WaitForMultipleObjects(2, eventos, FALSE, 0);

        switch (ret) {
        case WAIT_OBJECT_0: // evCLP_Exit
            return 0;

        case WAIT_OBJECT_0 + 1: // evCLPHotbox_PauseResume
            pausado = !pausado;
            printf("Thread Hotbox %s\n", pausado ? "PAUSADA" : "RETOMADA");
            ResetEvent(evCLPHotbox_PauseResume);
            break;

        case WAIT_TIMEOUT:

            break;

        default:
            printf("Erro: %d\n", GetLastError());
            return 1;
        }

        if (!pausado) { //Se a thread estiver permitida de rodar
            
            //Sleep(500);
            WaitForSingleObject(evTemporização, 500); // evento que nunca será setado apenas para bloquear a thread
            cria_msg_roda();
        }
        else {
            // Se pausado, verifica eventos mais frequentemente
            WaitForSingleObject(evTemporização, 100); // evento que nunca será setado apenas para bloquear a thread
            //Sleep(100);
        }
    }
    return 0;
}


//############## FUNÇÃO DA THREAD DE CAPTURA DE RODA QUENTE###############
DWORD WINAPI CapturaHotboxThread(LPVOID) {
    char mensagem[SMALL_MSG_LENGTH];

    printf("[Hotbox-Captura] Thread iniciada.\n");

    while (1) {
        if (WaitForSingleObject(evEncerraThreads, 0) == WAIT_OBJECT_0) {
            printf("[Hotbox-Captura] Thread encerrada.\n");
            return 0; // Encerra a thread
        }

        WaitForSingleObject(hMutexBufferRoda, INFINITE);//Conquista MUTEX para leitura da lista circular
               
        if (ReadFromRodaBuffer(mensagem)) {
            // Confirma se é uma mensagem tipo 99
            if (mensagem[8] == '9' && mensagem[9] == '9') {
                printf("\033[31mMensagem lida de Hotbox: %s\033[0m\n", mensagem);

            }
        }
        else {
            //Sleep(100); //reduz o consumo de CPU e ainda mantém a verificação do buffer com certa frequencia (10 vezes por segundo)
            WaitForSingleObject(evTemporização, 100); // evento que nunca será setado apenas para bloquear a thread
        }

        ReleaseMutex(hMutexBufferRoda); //Libera MUTEX
    }

    return 0;
}

//############## FUNÇÃO DA THREAD DE CAPTURA DE SINALIZAÇÃO FERROVIÁRIA ###############
DWORD WINAPI CapturaSinalizacaoThread(LPVOID) {
    char mensagem[MAX_MSG_LENGTH];

    printf("[Ferrovia-Captura] Thread iniciada.\n");

    while (1) {
        if (WaitForSingleObject(evEncerraThreads, 0) == WAIT_OBJECT_0) {
            printf("[Ferrovia-Captura] Thread encerrada.\n");
            return 0; // Encerra a thread
        }

        WaitForSingleObject(hMutexBufferFerrovia, INFINITE); //Conquista MUTEX para leitura da lista circular
        
        if (ReadFromFerroviaBuffer(mensagem)) {
            // Verifica se é mensagem tipo 00 (sinalização ferroviária)
            if (mensagem[8] == '0' && mensagem[9] == '0') {
                printf("\033[31mMensagem lida de Ferrovia: %s\033[0m\n", mensagem);
            }
        }
        else {
            WaitForSingleObject(evTemporização, 100); // evento que nunca será setado apenas para bloquear a thread
            //Sleep(100); // Evita uso intenso da CPU
        }

        ReleaseMutex(hMutexBufferFerrovia); //Libera MUTEX
    }

    return 0;
}

//############# FUNÇÃO MAIN DO SISTEMA ################
int main() {
    InitializeBuffers();

    HANDLE hCLPThreadFerrovia;
    HANDLE hCLPThreadRoda;
    HANDLE hCapturaHotboxThread;
    DWORD dwThreadId;
    hEvent_ferrovia = CreateEvent(NULL, TRUE, FALSE, L"EvTimeOutFerrovia");
    // Eventos de pausa/retomada
    evCLPHotbox_PauseResume = CreateEvent(NULL, FALSE, FALSE, L"EV_CLPH_PAUSE");
	evTemporização = CreateEvent(NULL, FALSE, FALSE, L"EV_TEMPORIZACAO"); // evento que nunca sera setado apenas para temporização
    evCLPFerrovia_PauseResume = CreateEvent(NULL, FALSE, FALSE, L"EV_CLPF_PAUSE");
    //evCLP_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_CLP_PAUSE");
    evFERROVIA_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_FERROVIA_PAUSE");
    evHOTBOX_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_HOTBOX_PAUSE"); 
    evVISUFERROVIA_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_VISUFERROVIA_PAUSE");
    evVISUHOTBOX_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_VISUHOTBOX_PAUSE");

    // Eventos de término
    evCLP_Exit = CreateEvent(NULL, TRUE, FALSE, L"EV_CLP_EXIT");
    evEncerraThreads = CreateEvent(NULL, TRUE, FALSE, L"EV_ENCERRA_THREADS"    );
    // Cria a thread CLP que escreve no buffer ferrovia
    hCLPThreadFerrovia = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION)CLPMsgFerrovia,
        NULL,
        0,
        (CAST_LPDWORD)&dwThreadId
    );

    if (hCLPThreadFerrovia) {
        printf("Thread CLP criada com ID=0x%x\n", dwThreadId);
    }

    // Cria a thread CLP que escreve no buffer roda
    hCLPThreadRoda = (HANDLE)_beginthreadex(
        NULL,
        0,
        (CAST_FUNCTION)CLPMsgRodaQuente,
        NULL,
        0,
        (CAST_LPDWORD)&dwThreadId
    );

    if (hCLPThreadRoda) {
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


    //##########COLOCA UM PATH GERAL###################
    WCHAR exePath[MAX_PATH];
    WCHAR hotboxesPath[MAX_PATH];
    WCHAR sinalizacaoPath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) {
        *lastSlash = L'\0';  // Trunca o caminho, removendo o nome do .exe
    }
    swprintf(hotboxesPath, MAX_PATH, L"%s\\VisualizaHotboxes.exe", exePath);
    swprintf(sinalizacaoPath, MAX_PATH, L"%s\\VisualizaSinalizacao.exe", exePath);

    //Para arrumar os bugs
    //wprintf(L"[DEBUG] Caminho VisualizaHotboxes: %s\n", hotboxesPath);
    //wprintf(L"[DEBUG] Caminho VisualizaSinalizacao: %s\n", sinalizacaoPath);


    //###### Criação de processo separado com novo console para Hotboxes #########
    if (CreateProcess(
        hotboxesPath, // Nome do executável do processo filho
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
        printf("Processo VisualizaHotboxes.exe criado com sucesso!\n");
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        printf("Erro ao criar processo VisualizaHotboxes.exe. Código do erro: %lu\n", GetLastError());
    }

    //##### Criação de processo separado com novo console para Sinalização Ferroviária #####
    if (CreateProcess(
        sinalizacaoPath, // Nome do executável do processo filho
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
        printf("Processo VisualizaSinalizacao.exe criado com sucesso!\n");
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        printf("Erro ao criar processo VisualizaSinalizacao.exe. Código do erro: %lu\n", GetLastError());
    }

    //############ LEITURA DO TECLADO ################
    BOOL executando = TRUE;
    BOOL clp_pausado = FALSE;
    BOOL ferrovia_pausado = FALSE;
    BOOL hotbox_pausado = FALSE;
    BOOL visuFerrovia_pausado = FALSE;
    BOOL visuHotbox_pausado = FALSE;
    while (executando) {
        
        if (_kbhit()) {
            int tecla = _getch();

            switch (tecla) {
            case 'c':
                clp_pausado = !clp_pausado;
                SetEvent(evCLPFerrovia_PauseResume); 
                SetEvent(evCLPHotbox_PauseResume);
                printf("CLP %s\n", clp_pausado ? "PAUSADO" : "RETOMADO");
                break;

            case 'd':
                ferrovia_pausado = !ferrovia_pausado;
                SetEvent(evFERROVIA_PauseResume);
                printf("Captura Ferrovia %s\n", ferrovia_pausado ? "PAUSADA" : "RETOMADA");
                break;

            case 'h':
                hotbox_pausado = !hotbox_pausado;
                SetEvent(evHOTBOX_PauseResume);
                printf("Captura Hotbox %s\n", hotbox_pausado ? "PAUSADA" : "RETOMADA");
                break;

            case 's':
                visuFerrovia_pausado = !visuFerrovia_pausado;
                SetEvent(evVISUFERROVIA_PauseResume);
                printf("Visualização Ferrovia %s\n", visuFerrovia_pausado ? "PAUSADA" : "RETOMADA");
                break;

            case 'q':
                visuHotbox_pausado = !visuHotbox_pausado;
                SetEvent(evVISUHOTBOX_PauseResume);
                printf("Visualização Hotbox %s\n", visuHotbox_pausado ? "PAUSADA" : "RETOMADA");
                break;

            case 27: // ESC
                printf("Encerrando todas as tarefas...\n");
                SetEvent(evCLP_Exit);
                SetEvent(evEncerraThreads);
                executando = FALSE;
                break;
            }
        }
        WaitForSingleObject(evTemporização, 1000); // evento que nunca será setado apenas para bloquear a thread
        //Sleep(1000); // Atualização periódica
    }

    // Limpeza
    WaitForSingleObject(hCLPThreadFerrovia, INFINITE);
	printf("Thread CLP Ferrovia terminou\n");
    WaitForSingleObject(hCLPThreadRoda, INFINITE);
    printf("Thread CLP roda terminou\n");
    WaitForSingleObject(hCapturaHotboxThread, INFINITE);
    WaitForSingleObject(hCapturaSinalizacaoThread, INFINITE);

    // Fecha handles das threads
    CloseHandle(hCLPThreadFerrovia);
    CloseHandle(hCLPThreadRoda);
    CloseHandle(hCapturaHotboxThread);
    CloseHandle(hCapturaSinalizacaoThread);

    // Fecha handles de eventos
    CloseHandle(hEvent_ferrovia);
    CloseHandle(hEvent_roda);
    CloseHandle(hBufferRodaCheio);
    CloseHandle(evCLPFerrovia_PauseResume);
    CloseHandle(evCLPHotbox_PauseResume);
    CloseHandle(evFERROVIA_PauseResume);
    CloseHandle(evHOTBOX_PauseResume);
    CloseHandle(evVISUFERROVIA_PauseResume);
    CloseHandle(evVISUHOTBOX_PauseResume);
    CloseHandle(evCLP_Exit);
    CloseHandle(evEncerraThreads);
    CloseHandle(evFERROVIA_Exit);
    CloseHandle(evHOTBOX_Exit);
    CloseHandle(evVISUFERROVIA_Exit);
    CloseHandle(evVISUHOTBOX_Exit);

    DestroyBuffers();
    CloseHandle(hMutexBufferRoda);
    CloseHandle(hMutexBufferFerrovia);

    //printf("\nPressione qualquer tecla para sair...\n");
    //_getch();

    return 0;
}