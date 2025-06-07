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
HANDLE evCLP_PauseResume, evFERROVIA_PauseResume, evHOTBOX_PauseResume;
HANDLE evVISUFERROVIA_PauseResume, evVISUHOTBOX_PauseResume;
HANDLE evCLP_Exit, evFERROVIA_Exit, evHOTBOX_Exit;
HANDLE evVISUFERROVIA_Exit, evVISUHOTBOX_Exit;
DWORD WINAPI CLPThread(LPVOID);


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

    HANDLE BlockingEvents[2] = { evCLP_Exit , evCLP_PauseResume };
    

    //COMENTADO POIS A PRIMEIRA ENTREGA DEVE UTILIZAR APENAS A FUNÇÃO SLEEP()
    // Inicializa os eventos e a fila de temporizadores
    //HANDLE hTimerQueue;
    //HANDLE hEvent_ferrovia;
    //HANDLE hEvent_roda;

    //BOOL status_queue;

    
    ////Cria fila de temporizadores
    //hTimerQueue = CreateTimerQueue();
    //if (hTimerQueue == NULL) {
    //    printf("Falha em CreateTimerQueue! Codigo =%d)\n", GetLastError());
    //    return 0;
    //}

    //// Enfileira o temporizador da roda quente, fora do DO WHILE porque tem temporização fixa e posso usar a temporização periódica
    //status_queue = CreateTimerQueueTimer(&hEvent_roda, hTimerQueue, (WAITORTIMERCALLBACK)cria_msg_roda,
    //    NULL, 500, 500, WT_EXECUTEDEFAULT);
    //if (!status_queue) {
    //    printf("Erro em CreateTimerQueueTimer [2]! Codigo = %d)\n", GetLastError());
    //    return 0;
    //}

    do {
        WaitForMultipleObjects(2, BlockingEvents, FALSE, INFINITE);
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

//############# FUNÇÃO CRIA MENSAGENS DE RODA QUENTE #############
DWORD WINAPI CLPMsgRodaQuente(LPVOID) { 
    HANDLE BlockingEvents[2] = { evCLP_Exit , evCLP_PauseResume };
    do {
        WaitForMultipleObjects(2, BlockingEvents, FALSE, INFINITE);

		Sleep(500); // Espera 500ms para criar uma nova mensagem de roda quente
		cria_msg_roda(); // Chama a função de criação da mensagem de roda quente
    } while (1);
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
    // Eventos de pausa/retomada
    evCLP_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_CLP_PAUSE");
    evFERROVIA_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_FERROVIA_PAUSE");
    evHOTBOX_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_HOTBOX_PAUSE");
    evVISUFERROVIA_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_VISUFERROVIA_PAUSE");
    evVISUHOTBOX_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_VISUHOTBOX_PAUSE");

    // Eventos de término
    evCLP_Exit = CreateEvent(NULL, TRUE, FALSE, L"EV_CLP_EXIT");
    evFERROVIA_Exit = CreateEvent(NULL, TRUE, FALSE, L"EV_FERROVIA_EXIT");
    evHOTBOX_Exit = CreateEvent(NULL, TRUE, FALSE, L"EV_HOTBOX_EXIT");
    evVISUFERROVIA_Exit = CreateEvent(NULL, TRUE, FALSE, L"EV_VISUFERROVIA_EXIT");
    evVISUHOTBOX_Exit = CreateEvent(NULL, TRUE, FALSE, L"EV_VISUHOTBOX_EXIT");
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

    //Leitura do Teclado
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
                SetEvent(evCLP_PauseResume);
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
                SetEvent(evFERROVIA_Exit);
                SetEvent(evHOTBOX_Exit);
                SetEvent(evVISUFERROVIA_Exit);
                SetEvent(evVISUHOTBOX_Exit);
                executando = FALSE;
                break;
            }
        }
        Sleep(1000); // Atualização periódica
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