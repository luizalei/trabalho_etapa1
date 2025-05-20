#include <windows.h>
#include <stdio.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include "circular_list.h"


#define BUFFER_SIZE 200  // Tamanho fixo da lista circular

//######### STRUCT CRIAÇÃO DE THREADS #########
typedef struct {
    const char* nome;   // Nome da thread 
    int id;             // ID ou outros parâmetros
} ThreadArgs;

CircularList lista_circular;

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

//######### STRUCT AGRUPA ARGUMENTOS CLP #########
typedef struct {
    ThreadArgs thread_args;
    int* sequencial;
} CLP_Args;

// Função para gerar timestamp no formato HH:MM:SS:MS
void gerar_timestamp(char* timestamp) {
    SYSTEMTIME time;
    GetLocalTime(&time);
    sprintf(timestamp, "%02d:%02d:%02d:%03d",
        time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
}

// Função para formatar a mensagem completa
void formatar_mensagem(char* buffer, const mensagem_ferrovia* msg) {
    sprintf(buffer, "%07d;%s;%d;%03d;%s;%d;%s",
        msg->nseq,
        msg->tipo,
        msg->diag,
        msg->remota,
        msg->id,
        msg->estado,
        msg->timestamp);
}

//FUNÇÃO DE SIMULAÇÃO DO CLP
void* CLP_thread_func(void* arg) {
    CLP_Args* args = (CLP_Args*)arg;
    printf("Thread %s (ID=%d) iniciou!\n", args->thread_args.nome, args->thread_args.id);

    // Simulação de tarefa da thread CLP
    int tempo_rodas = 500; //tempo fixo para envio de mensagens sobre rodas
    do {
		int tempo_ferrovia = 100 + rand() % (2000 - 100 + 1); //tempo aleatório para envio de mensagens sobre ferrovia
        char mensagem[41];
        int seq = __sync_fetch_and_add(args->sequencial, 1);  // Incrementa o sequencial de forma segura
        mensagem_ferrovia msg = {
        msg.nseq = seq,  // 1-9999999
        msg.tipo = "00",
        msg.diag = rand() % 10,           // 0-9
        msg.remota = rand() % 1000,       // 000-999
        msg.id = "STN-0023",              // Exemplo fixo
        msg.estado = rand() % 2 + 1       // 1 ou 2
        };
        gerar_timestamp(msg.timestamp);

        formatar_mensagem(mensagem, &msg);

        cb_push(&lista_circular, mensagem_ferrovia); //insere um dado na lista circular


	} while (1); // TROCAR POR CONDIÇÃO DE FINALIZAÇÃO DA THREAD;

    printf("Thread %s terminou.\n", args->nome);
    pthread_exit(NULL); // Finaliza a thread


    return NULL;
}

int main() {
    cb_init(&lista_circular); // Inicializa a lista circular
   
//########################## CRIA THREAD DE SIMULAÇÃO DO CLP ##########################
    pthread_t CLP_thread; // thread do CLP
    int sequencial = 0;
    // Empacota todos os argumentos
    CLP_Args args = {
        args.thread_args = { "CLP", 1 },
        args.sequencial = &sequencial
    };
    if (pthread_create(&CLP_thread, NULL, CLP_thread_func, &args) != 0) {
        perror("Erro ao criar thread CLP");
        return EXIT_FAILURE;
    }
   printf("Thread CLP criada com sucesso!\n");
    

    return EXIT_SUCCESS;
}