// visualiza_hotboxes.cpp

#include <windows.h>
#include <stdio.h>

DWORD WINAPI ThreadVisualizaHotboxes(LPVOID) {
    while (1) {
        printf("Thread de Visualizacao de Hotboxes funcionando\n");
        Sleep(3000); // 3 segundos
    }
    return 0;
}

int main() {
    HANDLE hThread = CreateThread(
        NULL, 0, ThreadVisualizaHotboxes, NULL, 0, NULL);

    if (hThread == NULL) {
        printf("Erro ao criar a thread.\n");
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    return 0;
}
