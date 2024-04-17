#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <conio.h>

// Define constantes
#define PESO_ESTEIRA1 5
#define PESO_ESTEIRA2 2
#define INTERVALO_ESTEIRA1 2000        // em milissegundos
#define INTERVALO_ESTEIRA2 1000        // em milissegundos
#define INTERVALO_EXIBICAO 2000        // Intervalo de tempo para atualização da exibição (em milissegundos)
#define INTERVALO_ATUALIZACAO_PESO 500 // Intervalo de itens para atualizar o peso

// Estrutura para passar os identificadores dos pipes para a função exibicao
typedef struct
{
    HANDLE pipe_esteira1;
    HANDLE pipe_esteira2;
} PipeIDs;

// Função para simular a esteira 1
void esteira1(void *pipe_fd)
{
    HANDLE pipe = (HANDLE)pipe_fd;

    int contagem = 0;
    int peso_total = 0;
    while (1)
    {
        contagem++;
        peso_total += PESO_ESTEIRA1;
        WriteFile(pipe, &contagem, sizeof(contagem), NULL, NULL);
        WriteFile(pipe, &peso_total, sizeof(peso_total), NULL, NULL);

        Sleep(INTERVALO_ESTEIRA1);
    }
}

// Função para simular a esteira 2
void esteira2(void *pipe_fd)
{
    HANDLE pipe = (HANDLE)pipe_fd;

    int contagem = 0;
    int peso_total = 0;
    while (1)
    {
        contagem += 2;
        peso_total = contagem * PESO_ESTEIRA2;
        WriteFile(pipe, &contagem, sizeof(contagem), NULL, NULL);
        WriteFile(pipe, &peso_total, sizeof(peso_total), NULL, NULL);

        Sleep(INTERVALO_ESTEIRA2);
    }
}

// Função para exibir informações sobre as esteiras
void exibicao(void *arg)
{
    PipeIDs *pipes = (PipeIDs *)arg;
    HANDLE pipe_esteira1 = pipes->pipe_esteira1;
    HANDLE pipe_esteira2 = pipes->pipe_esteira2;

    int contagem_total = 0;
    int ultimaContagemPeso = 0;
    int peso_total = 0;
    int ultima_contagem_atualizacao = 0;

    while (1)
    {
        DWORD start = GetTickCount();
        int contagem1, peso1, contagem2, peso2;
        ReadFile(pipe_esteira1, &contagem1, sizeof(contagem1), NULL, NULL);
        ReadFile(pipe_esteira1, &peso1, sizeof(peso1), NULL, NULL);
        ReadFile(pipe_esteira2, &contagem2, sizeof(contagem2), NULL, NULL);
        ReadFile(pipe_esteira2, &peso2, sizeof(peso2), NULL, NULL);

        contagem_total = contagem1 + contagem2;

        if ((ultimaContagemPeso + INTERVALO_ATUALIZACAO_PESO) <= contagem_total)
        {
            peso_total = peso1 + peso2;
            ultimaContagemPeso = ultimaContagemPeso + INTERVALO_ATUALIZACAO_PESO;
            printf("Peso atualizado: %d\n", peso_total);
        }

        printf("Esteira 1: Itens = %d, Peso = %d\n", contagem1, peso1);
        printf("Esteira 2: Itens = %d, Peso = %d\n", contagem2, peso2);
        printf("Total: Itens = %d, Peso = %d\n", contagem_total, peso_total);
        printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");

        Sleep(INTERVALO_EXIBICAO);
    }
}

int main()
{
    HANDLE pipe_esteira1[2], pipe_esteira2[2];
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    if (!CreatePipe(&pipe_esteira1[0], &pipe_esteira1[1], &sa, 0) ||
        !CreatePipe(&pipe_esteira2[0], &pipe_esteira2[1], &sa, 0))
    {
        perror("Erro ao criar pipes");
        exit(EXIT_FAILURE);
    }

    // Estrutura para passar os identificadores dos pipes para a função exibicao
    PipeIDs pipes = {pipe_esteira1[0], pipe_esteira2[0]};

    // Cria threads para as esteiras e para exibir informações
    _beginthread(esteira1, 0, (void *)pipe_esteira1[1]);
    _beginthread(esteira2, 0, (void *)pipe_esteira2[1]);
    _beginthread(exibicao, 0, (void *)&pipes);

    // Aguarda o término das threads
    Sleep(INFINITE);

    CloseHandle(pipe_esteira1[0]);
    CloseHandle(pipe_esteira1[1]);
    CloseHandle(pipe_esteira2[0]);
    CloseHandle(pipe_esteira2[1]);

    return 0;
}
