#include <windows.h>
#include <stdio.h>

HANDLE evVISUFERROVIA_PauseResume;
HANDLE evVISUFERROVIA_Exit;
HANDLE evVISUFERROVIATemporiza��o;
HANDLE evEncerraThreads;

DWORD WINAPI ThreadVisualizaSinalizacao(LPVOID) {
        HANDLE eventos[2] = { evVISUFERROVIA_PauseResume, evEncerraThreads };
    BOOL pausado = FALSE;

    printf("Thread de Visualizacao de Sinalizacao iniciada\n");

    while (1) {
        if (!pausado) {
            printf("Thread de Visualizacao de Sinalizacao funcionando\n");
            WaitForSingleObject(evVISUFERROVIATemporiza��o, 1000); // evento que nunca ser� setado apenas para bloquear a thread
            //Sleep(1000);  // Atualiza��o peri�dica
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
                printf("[Sinalizacao] Retomando execu��o.\n");
            }
            ResetEvent(evVISUFERROVIA_PauseResume);
            break;

        case WAIT_OBJECT_0 + 1:  // evEncerraThreads
            printf("[Sinalizacao] Evento de sa�da recebido. Encerrando thread.\n");
            return 0;

        default:
            break; // Nenhum evento sinalizado
        }

        // Se pausado, entra em espera bloqueante at� algo acontecer
        while (pausado) {
            DWORD r = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) { // Toggle pausa
                pausado = FALSE;
                printf("[Sinalizacao] Retomando execu��o.\n");
                ResetEvent(evVISUFERROVIA_PauseResume);
                break;
            }
            else if (r == WAIT_OBJECT_0 + 1) { // Evento de sa�da
                printf("[Sinalizacao] Evento de sa�da recebido. Encerrando thread.\n");
                return 0;
            }
        }
    }

    return 0;
}

int main() {
	evVISUFERROVIA_PauseResume = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_VISUFERROVIA_PAUSE");
    evVISUFERROVIA_Exit = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_VISUFERROVIA_EXIT"); 
	evEncerraThreads = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_ENCERRA_THREADS");
    evVISUFERROVIATemporiza��o = CreateEvent(NULL, FALSE, FALSE, L"EV_VISUFERROVIA_TEMPORIZACAO"); // evento que nunca ser� setado apenas para temporiza��o

    HANDLE hThread = CreateThread(NULL, 0, ThreadVisualizaSinalizacao, NULL, 0, NULL);
    if (hThread == NULL) {
        printf("Erro ao criar a thread.\n");
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
    CloseHandle(evVISUFERROVIA_PauseResume);
    CloseHandle(evVISUFERROVIA_Exit);
	CloseHandle(evVISUFERROVIATemporiza��o);
	CloseHandle(evEncerraThreads);

    return 0;
}

