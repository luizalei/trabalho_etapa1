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

// === Mensagens ===
typedef struct {
    int32_t nseq;
    char tipo[3];       // "00"
    int8_t diag;
    int16_t remota;
    char id[9];
    int8_t estado;
    char timestamp[13];
} mensagem_ferrovia;

typedef struct {
    int32_t nseq;
    char tipo[3];       // "99"
    char id[9];
    int8_t estado;
    char timestamp[13];
} mensagem_hotbox;

// === Funções auxiliares ===
void gerar_timestamp(char* timestamp) {
    SYSTEMTIME time;
    GetLocalTime(&time);
    sprintf_s(timestamp, 13, "%02d:%02d:%02d:%03d",
        time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
}

void formatar_mensagem_ferrovia(char* buffer, size_t buffer_size, const mensagem_ferrovia* msg) {
    sprintf_s(buffer, buffer_size, "%07d;%s;%d;%03d;%s;%d;%s",
        msg->nseq, msg->tipo, msg->diag, msg->remota, msg->id, msg->estado, msg->timestamp);
}

void formatar_mensagem_hotbox(char* buffer, size_t buffer_size, const mensagem_hotbox* msg) {
    sprintf_s(buffer, buffer_size, "%07d;%s;%s;%d;%s",
        msg->nseq, msg->tipo, msg->id, msg->estado, msg->timestamp);
}

int gcounter_ferrovia = 0;
int gcounter_hotbox = 0;

// === Eventos de controle ===
HANDLE evHOT_PauseResume, evHOT_Exit;
HANDLE evHWQ_PauseResume, evHWQ_Exit;

// === Pipe para comunicação entre threads ===
HANDLE hPipeReadHotbox = NULL;
HANDLE hPipeWriteHotbox = NULL;

// === Thread CLP ===
DWORD WINAPI CLpThread(LPVOID) {
    auto ultimoHotbox = std::chrono::steady_clock::now();

    while (true) {
        mensagem_ferrovia msg_f;
        char buffer_f[100];

        msg_f.nseq = ++gcounter_ferrovia;
        strcpy_s(msg_f.tipo, sizeof(msg_f.tipo), "00");
        msg_f.diag = rand() % 2;
        msg_f.remota = rand() % 1000;
        int numero = 1 + (rand() % 100);
        sprintf_s(msg_f.id, sizeof(msg_f.id), "SIN-%04d", numero);
        msg_f.estado = (rand() % 2) + 1;
        gerar_timestamp(msg_f.timestamp);
        if (msg_f.diag == 1) {
            strcpy_s(msg_f.id, sizeof(msg_f.id), "XXXXXXXX");
            msg_f.estado = 0;
        }
        formatar_mensagem_ferrovia(buffer_f, sizeof(buffer_f), &msg_f);
        WriteToBuffer(buffer_f, 40);

        auto agora = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(agora - ultimoHotbox).count() >= 500) {
            mensagem_hotbox msg_h;
            char buffer_h[100];
            msg_h.nseq = ++gcounter_hotbox;
            strcpy_s(msg_h.tipo, sizeof(msg_h.tipo), "99");
            int idnum = 1 + (rand() % 100);
            sprintf_s(msg_h.id, sizeof(msg_h.id), "HWD-%04d", idnum);
            msg_h.estado = rand() % 2;
            gerar_timestamp(msg_h.timestamp);
            formatar_mensagem_hotbox(buffer_h, sizeof(buffer_h), &msg_h);
            WriteToBuffer(buffer_h, 34);
            ultimoHotbox = agora;
        }

        int delay = (rand() % 1901) + 100;
        Sleep(delay);
    }
    return 0;
}

// === Thread de Captura de Hotbox ===
DWORD WINAPI ThreadCapturaHotbox(LPVOID) {
    char mensagem[100];
    size_t msg_size;

    printf("[Hotbox-Captura] Thread iniciada.\n");

    while (true) {
        if (WaitForSingleObject(evHOT_Exit, 0) == WAIT_OBJECT_0) return 0;
        WaitForSingleObject(evHOT_PauseResume, 0);

        if (ReadFromBuffer(mensagem, &msg_size)) {
            if (msg_size == 34 && mensagem[8] == '9' && mensagem[9] == '9') {
                DWORD bytesWritten;
                WriteFile(hPipeWriteHotbox, mensagem, (DWORD)msg_size, &bytesWritten, NULL);
                printf("[Hotbox-Captura] Enviado para visualização: %s\n", mensagem);
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    return 0;
}

// === Thread de Visualização de Hotbox ===
DWORD WINAPI ThreadVisualizacaoHotbox(LPVOID) {
    char buffer[100];
    DWORD bytesRead;
    printf("[Hotbox-Visualizacao] Thread iniciada.\n");

    while (true) {
        if (WaitForSingleObject(evHWQ_Exit, 0) == WAIT_OBJECT_0) return 0;
        WaitForSingleObject(evHWQ_PauseResume, 0);

        BOOL sucesso = ReadFile(hPipeReadHotbox, buffer, sizeof(buffer), &bytesRead, NULL);
        if (sucesso && bytesRead > 0) {
            buffer[bytesRead] = '\0';

            // Verifica tipo da mensagem
            if (buffer[8] == '9' && buffer[9] == '9') {
                // Mensagem hotbox: NSEQ;99;ID;ESTADO;TIMESTAMP
                char* token;
                char* rest = buffer;

                char* nseq = strtok_s(rest, ";", &rest);
                strtok_s(NULL, ";", &rest); // skip tipo
                char* id = strtok_s(NULL, ";", &rest);
                char* estado = strtok_s(NULL, ";", &rest);
                char* timestamp = strtok_s(NULL, ";", &rest);

                int est = atoi(estado);
                if (est == 0)
                    printf("%.8s NSEQ: %s DETECTOR: %s TEMP. DENTRO DA FAIXA\n", timestamp, nseq, id);
                else
                    printf("%.8s NSEQ: %s DETECTOR: %s RODA QUENTE DETECTADA\n", timestamp, nseq, id);
            }
            else if (buffer[8] == '0' && buffer[9] == '0') {
                // Mensagem de falha de hardware: NSEQ;00;DIAG;REMOTA;ID;ESTADO;TIMESTAMP
                char* token;
                char* rest = buffer;

                char* nseq = strtok_s(rest, ";", &rest);
                strtok_s(NULL, ";", &rest); // skip tipo
                char* diag = strtok_s(NULL, ";", &rest);
                char* remota = strtok_s(NULL, ";", &rest);
                strtok_s(NULL, ";", &rest); // skip id
                strtok_s(NULL, ";", &rest); // skip estado
                char* timestamp = strtok_s(NULL, ";", &rest);

                if (atoi(diag) == 1) {
                    printf("%s NSEQ: %s REMOTA: %s FALHA DE HARDWARE\n", timestamp, nseq, remota);
                }
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    return 0;
}

// === Main ===
int main() {
    srand((unsigned int)time(NULL));
    InitializeBuffer();

    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    CreatePipe(&hPipeReadHotbox, &hPipeWriteHotbox, &saAttr, 0);

    evHOT_PauseResume = CreateEvent(NULL, FALSE, FALSE, NULL);
    evHOT_Exit = CreateEvent(NULL, TRUE, FALSE, NULL);
    evHWQ_PauseResume = CreateEvent(NULL, FALSE, FALSE, NULL);
    evHWQ_Exit = CreateEvent(NULL, TRUE, FALSE, NULL);

    HANDLE hCLPThread = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)CLpThread, NULL, 0, NULL);
    HANDLE hThreadHotbox = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)ThreadCapturaHotbox, NULL, 0, NULL);
    HANDLE hThreadVisuHotbox = (HANDLE)_beginthreadex(NULL, 0, (CAST_FUNCTION)ThreadVisualizacaoHotbox, NULL, 0, NULL);

    printf("Digite 'h' para pausar/retomar Captura Hotbox, 'q' para Visualizacao ou ESC para sair.\n");

    while (true) {
        int ch = _getch();
        if (ch == 'h') SetEvent(evHOT_PauseResume);
        else if (ch == 'q') SetEvent(evHWQ_PauseResume);
        else if (ch == 27) {
            SetEvent(evHOT_Exit);
            SetEvent(evHWQ_Exit);
            break;
        }
    }

    WaitForSingleObject(hThreadHotbox, INFINITE);
    WaitForSingleObject(hThreadVisuHotbox, INFINITE);
    CloseHandle(hThreadHotbox);
    CloseHandle(hThreadVisuHotbox);
    CloseHandle(hPipeReadHotbox);
    CloseHandle(hPipeWriteHotbox);
    DestroyBuffer();
    return 0;
}
