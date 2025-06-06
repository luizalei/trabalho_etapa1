#include <windows.h>
#include <stdio.h>

HANDLE evVISUFERROVIA_PauseResume;
HANDLE evVISUFERROVIA_Exit;

DWORD WINAPI ThreadVisualizaSinalizacao(LPVOID) {
    HANDLE eventos[2] = { evVISUFERROVIA_PauseResume, evVISUFERROVIA_Exit };
    BOOL pausado = FALSE;

    printf("Thread de Visualizacao de Sinalizacao iniciada\n");

    while (1) {
        if (!pausado) {
            printf("Thread de Visualizacao de Sinalizacao funcionando\n");
            Sleep(1000);  // Atualização periódica
        }

        // Verifica os dois eventos simultaneamente (sem bloquear)
        DWORD result = WaitForMultipleObjects(2, eventos, FALSE, 0);

        switch (result) {
        case WAIT_OBJECT_0:  // evVISUFERROVIA_PauseResume
            pausado = !pausado;
            if (pausado) {
                printf("[Sinalizacao] Thread pausada. Aguardando retomada...\n");
            }
            else {
                printf("[Sinalizacao] Retomando execução.\n");
            }
            ResetEvent(evVISUFERROVIA_PauseResume);
            break;

        case WAIT_OBJECT_0 + 1:  // evVISUFERROVIA_Exit
            printf("[Sinalizacao] Evento de saída recebido. Encerrando thread.\n");
            return 0;

        default:
            break; // Nenhum evento sinalizado
        }

        // Se pausado, entra em espera bloqueante até algo acontecer
        while (pausado) {
            DWORD r = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) { // Toggle pausa
                pausado = FALSE;
                printf("[Sinalizacao] Retomando execução.\n");
                ResetEvent(evVISUFERROVIA_PauseResume);
                break;
            }
            else if (r == WAIT_OBJECT_0 + 1) { // Evento de saída
                printf("[Sinalizacao] Evento de saída recebido. Encerrando thread.\n");
                return 0;
            }
        }
    }

    return 0;
}

int main() {
    evVISUFERROVIA_PauseResume = CreateEvent(NULL, TRUE, FALSE, L"EV_VISUFERROVIA_PAUSE");
    evVISUFERROVIA_Exit = CreateEvent(NULL, TRUE, FALSE, L"EV_VISUFERROVIA_EXIT");

    HANDLE hThread = CreateThread(NULL, 0, ThreadVisualizaSinalizacao, NULL, 0, NULL);
    if (hThread == NULL) {
        printf("Erro ao criar a thread.\n");
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
    CloseHandle(evVISUFERROVIA_PauseResume);
    CloseHandle(evVISUFERROVIA_Exit);

    return 0;
}

