#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>

// Define constants
#define ESTEIRA1_PESO 5        
#define ESTEIRA2_PESO 2  
#define ESTEIRA1_INTERVALO 2000  // em milissegundos
#define ESTEIRA2_INTERVALO 1000  // em milissegundos
#define DISPLAY_INTERVAL 2000     // Intervalo de tempo para atualização da exibição (em milissegundos)
#define UPDATE_peso_INTERVAL 500 // Intervalo de itens para atualizar o peso total

// Define data structures
typedef struct {
    int peso; 
    int count;       
    int total_peso;
} Esteira;

// Define global variables
Esteira esteira1 = {ESTEIRA1_PESO, 0, 0};
Esteira esteira2 = {ESTEIRA2_PESO, 0, 0};
HANDLE esteira_mutex; // Mutex para exclusão mútua

// Função para simular a esteira 1
DWORD WINAPI Esteira1_thread(LPVOID lpParam) {
    while (1) {
        WaitForSingleObject(esteira_mutex, INFINITE); 
        esteira1.count++;                  
        esteira1.total_peso += ESTEIRA1_PESO; 
        ReleaseMutex(esteira_mutex);

        Sleep(ESTEIRA1_INTERVALO); 
    }
}

void aguardarTecla()
{
    // Aguarda até que uma tecla seja pressionada
    if (_kbhit())
    {
        exit(0); // Encerra o programa
    }
}

// Função para simular a esteira 2
DWORD WINAPI esteira2_thread(LPVOID lpParam) {
    while (1) {
        WaitForSingleObject(esteira_mutex, INFINITE);
        esteira2.count++;                  
        esteira2.total_peso += ESTEIRA2_PESO; 
        ReleaseMutex(esteira_mutex);

        Sleep(ESTEIRA2_INTERVALO);
    }
}

DWORD WINAPI display_thread(LPVOID lpParam) {
    int total_count = 0;    // Número total de itens em ambas as esteiras
    int total_peso = 0;   // Peso total transportado por ambas as esteiras
    int last_total_count = 0; // Contagem total da última atualização
    int last_total_peso = 0; // Peso total da última atualização
    while (1) {
        WaitForSingleObject(esteira_mutex, INFINITE); // Bloqueia o mutex para acesso seguro às variáveis compartilhadas
        total_count = esteira1.count + esteira2.count;
        total_peso = esteira1.total_peso + esteira2.total_peso;
        ReleaseMutex(esteira_mutex); // Libera o mutex após o acesso às variáveis compartilhadas

        // Verifica se é hora de atualizar o peso total e exibe uma mensagem de atualização
        if (total_count - last_total_count >= UPDATE_peso_INTERVAL) {
            printf("peso updated: %d\n", total_peso);
            last_total_count = total_count;
            last_total_peso = total_peso;
        }

        // Exibe as informações sobre as esteiras e o peso total
        printf("Esteira 1: Items = %d, peso = %d\n", esteira1.count, esteira1.total_peso);
        printf("Esteira 2: Items = %d, peso = %d\n", esteira2.count, esteira2.total_peso);
        printf("Total: Items = %d, peso = %d\n", total_count, last_total_peso);

        printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
        Sleep(DISPLAY_INTERVAL); 

        // Verifica se uma tecla foi pressionada
        aguardarTecla();
    }
}


int main() {
    HANDLE esteira1_thread_handle, esteira2_thread_handle, display_thread_handle;

    // Inicializa o mutex
    esteira_mutex = CreateMutex(NULL, FALSE, NULL);

    // Cria threads para as esteiras e para exibir informações
    esteira1_thread_handle = CreateThread(NULL, 0, Esteira1_thread, NULL, 0, NULL);
    esteira2_thread_handle = CreateThread(NULL, 0, esteira2_thread, NULL, 0, NULL);
    display_thread_handle = CreateThread(NULL, 0, display_thread, NULL, 0, NULL);

    // Aguarda o término das threads
    WaitForSingleObject(esteira1_thread_handle, INFINITE);
    WaitForSingleObject(esteira2_thread_handle, INFINITE);
    WaitForSingleObject(display_thread_handle, INFINITE);

    // Libera o mutex
    CloseHandle(esteira_mutex);

    return 0; 
}