// visualiza_hotboxes.cpp

#include <windows.h>
#include <stdio.h>

DWORD WINAPI ThreadVisualizaSinalizacao(LPVOID) {
    while (1) {
        printf("Thread de Visualização de Sinalizacao Ferroviaria funcionando\n");
        Sleep(3000); // 3 segundos
    }
    return 0;
}

int main() {
    HANDLE hThread2 = CreateThread(
        NULL, 0, ThreadVisualizaSinalizacao, NULL, 0, NULL);

    if (hThread2 == NULL) {
        printf("Erro ao criar a thread.\n");
        return 1;
    }

    WaitForSingleObject(hThread2, INFINITE);
    CloseHandle(hThread2);
    return 0;
}
