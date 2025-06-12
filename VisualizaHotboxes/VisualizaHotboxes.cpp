#include <windows.h>
#include <stdio.h>


HANDLE evVISUHOTBOX_PauseResume;
HANDLE evVISUHOTBOX_Exit;
HANDLE evVISUHOTBOXTemporização;
HANDLE evEncerraThreads; // Necessário para sincronização com a main e fechamento do console

DWORD WINAPI ThreadVisualizaHotboxes(LPVOID) {
    HANDLE eventos[2] = { evVISUHOTBOX_PauseResume, evEncerraThreads };
    BOOL pausado = FALSE;

    printf("Thread de Visualizacao de Hotboxes iniciada\n");

    while (1) {
        if (!pausado) {
            printf("Thread de Visualizacao de Hotboxes funcionando\n");
            const char* mensagens[20] = {
                "Desvio atuado",
                "Sinaleiro em PARE",
                "Sinaleiro em VIA LIVRE",
                "Ocorrencia na via",
                "Sensor inativo",
                "Veiculo detectado",
                "Barreira abaixada",
                "Barreira levantada",
                "Desvio nao confirmado",
                "Sensor com falha",
                "Via ocupada",
                "Via livre",
                "Alarme de intrusao",
                "Alimentacao normal",
                "Alimentacao interrompida",
                "Sinaleiro apagado",
                "Controle manual ativado",
                "Controle automatico ativo",
                "Velocidade excedida",
                "Falha de comunicacao"
            };

            // Sorteia um índice aleatório entre 0 e 19
            int indice = rand() % 20;

            // Exibe a mensagem sorteada
            printf("Mensagem sorteada: %s\n", mensagens[indice]);

            //Usar essa variavel 'mensagens[indice]' no lugar do estado quando for printar a mensagem recebida

            WaitForSingleObject(evVISUHOTBOXTemporização, 1000); // evento que nunca será setado apenas para bloquear a thread
            //Sleep(1000);  // Atualização a cada segundo
        }

        // Aguarda por qualquer evento sinalizado
        DWORD result = WaitForMultipleObjects(2, eventos, FALSE, 0);

        switch (result) {
        case WAIT_OBJECT_0:  // evVISUHOTBOX_PauseResume
            pausado = !pausado;
            if (pausado) {
                printf("[Hotboxes] Thread pausada. Aguardando retomada...\n");
            }
            else {
                printf("[Hotboxes] Retomando execução.\n");
            }
            // Reseta evento para o próximo toggle
            ResetEvent(evVISUHOTBOX_PauseResume);
            break;

        case WAIT_OBJECT_0 + 1:  // evEncerraThreads
            printf("[Hotboxes] Evento de saída recebido. Encerrando thread.\n");
                        return 0;

        default:
            break; // Nenhum evento sinalizado, segue o loop
        }

        // Se está pausado, espera até um evento ser sinalizado (pause/exit)
        while (pausado) {
            DWORD r = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) { // Toggle pausa
                pausado = FALSE;
                printf("[Hotboxes] Retomando execução.\n");
                ResetEvent(evVISUHOTBOX_PauseResume);
                break;
            }
            else if (r == WAIT_OBJECT_0 + 1) { // Sair
                printf("[Hotboxes] Evento de saída recebido. Encerrando thread.\n");
                return 0;
            }
        }
    }

    return 0;
}



int main() {
    // Cria os eventos com os mesmos nomes do processo principal
   	evVISUHOTBOX_PauseResume = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_VISUHOTBOX_PAUSE");
    evVISUHOTBOX_Exit = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_VISUHOTBOX_EXIT"); 
   	evEncerraThreads = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_ENCERRA_THREADS");

    evVISUHOTBOXTemporização = CreateEvent(NULL, FALSE, FALSE, L"EV_VISUHOTBOX_TEMPORIZACAO"); // evento que nunca será setado apenas para temporização

    HANDLE hThread = CreateThread(NULL, 0, ThreadVisualizaHotboxes, NULL, 0, NULL);

    if (hThread == NULL) {
        printf("Erro ao criar a thread.\n");
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);

    // Limpeza
    CloseHandle(hThread);
    CloseHandle(evVISUHOTBOX_PauseResume);
    CloseHandle(evVISUHOTBOX_Exit);
	CloseHandle(evVISUHOTBOXTemporização);
	CloseHandle(evEncerraThreads);

    return 0;
}