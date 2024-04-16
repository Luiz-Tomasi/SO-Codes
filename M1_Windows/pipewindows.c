#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>

// Define constantes
#define PESO_ESTEIRA1 5
#define PESO_ESTEIRA2 2
#define INTERVALO_ESTEIRA1 2000   // em milissegundos
#define INTERVALO_ESTEIRA2 1000   // em milissegundos
#define INTERVALO_EXIBICAO 2000   // Intervalo de tempo para atualização da exibição (em milissegundos)
#define INTERVALO_ATUALIZACAO_PESO 5 // Intervalo de itens para atualizar o peso total

// Define estruturas de dados
typedef struct
{
    HANDLE handle_esteira;
    HANDLE handle_esteira2;
    int peso;
    int intervalo;
} ArgumentosEsteira;

void aguardarTecla()
{
    // Aguarda até que uma tecla seja pressionada
    if (_kbhit())
    {
        exit(0); // Encerra o programa
    }
}

// Escreve continuamente itens na esteira
void funcao_esteira(ArgumentosEsteira *args)
{
    int peso_temporario;

    while (1)
    {
        peso_temporario = args->peso;

        // Escreve na esteira 1
        if (!WriteFile(args->handle_esteira, &peso_temporario, sizeof(int), NULL, NULL))
        {
            printf("Erro ao escrever na esteira 1\n");
        }
        else
        {
            printf("Escrito na esteira 1: peso = %d\n", peso_temporario);
        }

        // Escreve na esteira 2
        if (!WriteFile(args->handle_esteira2, &peso_temporario, sizeof(int), NULL, NULL))
        {
            printf("Erro ao escrever na esteira 2\n");
        }
        else
        {
            printf("Escrito na esteira 2: peso = %d\n", peso_temporario);
        }

        Sleep(args->intervalo);

        // Verifica se uma tecla foi pressionada
        aguardarTecla();
    }
}

void funcao_exibicao(LPVOID lpParam)
{
    ArgumentosEsteira *args = (ArgumentosEsteira *)lpParam;
    int total_itens_esteira1 = 0;
    int total_itens_esteira2 = 0;
    int peso_total = 0;
    int ultimo_peso_total = 0;
    int peso_item;
    DWORD bytes_lidos;

    while (1)
    {
        // Lê da esteira 1
        if (ReadFile(args->handle_esteira, &peso_item, sizeof(int), NULL, NULL))
        {
            printf("Lido da esteira 1: peso = %d\n", peso_item);
            total_itens_esteira1++;
            peso_total += peso_item;
        }

        // Lê da esteira 2
        if (ReadFile(args->handle_esteira2, &peso_item, sizeof(int), NULL, NULL))
        {
            printf("Lido da esteira 2: peso = %d\n", peso_item);
            total_itens_esteira2++;
            peso_total += peso_item;
        }

        // Atualiza o peso total se necessário
        if ((total_itens_esteira1 + total_itens_esteira2) % INTERVALO_ATUALIZACAO_PESO == 0 && peso_total != ultimo_peso_total)
        {
            printf("Peso atualizado: %d\n", peso_total);
            ultimo_peso_total = peso_total;
        }

        // Exibe as informações das esteiras
        printf("Esteira 1: Itens = %d\n", total_itens_esteira1);
        printf("Esteira 2: Itens = %d\n", total_itens_esteira2);
        printf("Total: Peso = %d\n", ultimo_peso_total);
        printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");

        Sleep(INTERVALO_EXIBICAO);

        // Verifica se uma tecla foi pressionada
        aguardarTecla();
    }
}

int main()
{
    HANDLE leitura_esteira1, escrita_esteira1;
    HANDLE leitura_esteira2, escrita_esteira2;
    HANDLE handle_thread_exibicao;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    // Cria os pipes
    CreatePipe(&leitura_esteira1, &escrita_esteira1, &sa, 0);
    CreatePipe(&leitura_esteira2, &escrita_esteira2, &sa, 0);

    // Cria a thread de exibição
    ArgumentosEsteira args_exibicao = {leitura_esteira1, leitura_esteira2, 0, 0}; // Use intervalo = 0 para a thread de exibição
    handle_thread_exibicao = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)funcao_exibicao, &args_exibicao, 0, NULL);

    // Cria as threads das esteiras
    ArgumentosEsteira args_esteira1 = {escrita_esteira1, escrita_esteira2, PESO_ESTEIRA1, INTERVALO_ESTEIRA1};
    ArgumentosEsteira args_esteira2 = {escrita_esteira2, escrita_esteira1, PESO_ESTEIRA2, INTERVALO_ESTEIRA2};

    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)funcao_esteira, &args_esteira1, 0, NULL);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)funcao_esteira, &args_esteira2, 0, NULL);

    // Espera a thread de exibição terminar
    WaitForSingleObject(handle_thread_exibicao, INFINITE);

    // Fecha os handles
    CloseHandle(leitura_esteira1);
    CloseHandle(escrita_esteira1);
    CloseHandle(leitura_esteira2);
    CloseHandle(escrita_esteira2);

    return 0;
}