#include <windows.h>
#include <stdio.h>

HANDLE evVISUHOTBOX_PauseResume;
HANDLE evVISUHOTBOX_Exit;

DWORD WINAPI ThreadVisualizaHotboxes(LPVOID) {
    HANDLE eventos[2] = { evVISUHOTBOX_PauseResume, evVISUHOTBOX_Exit };
    BOOL pausado = FALSE;

    printf("Thread de Visualizacao de Hotboxes iniciada\n");

    while (1) {
        if (!pausado) {
            printf("Thread de Visualizacao de Hotboxes funcionando\n");
            Sleep(1000);  // Atualização a cada segundo
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

        case WAIT_OBJECT_0 + 1:  // evVISUHOTBOX_Exit
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
    evVISUHOTBOX_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_VISUHOTBOX_PAUSE");
    evVISUHOTBOX_Exit = CreateEvent(NULL, TRUE, FALSE, L"EV_VISUHOTBOX_EXIT");

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

    return 0;
}