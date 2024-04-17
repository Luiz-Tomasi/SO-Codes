#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h> // Para usar a função time()

// Define constantes
#define PESO_ESTEIRA1 5
#define PESO_ESTEIRA2 2
#define INTERVALO_ESTEIRA1 2
#define INTERVALO_ESTEIRA2 1
#define INTERVALO_EXIBICAO 2             // Intervalo de tempo para atualização da exibição (em segundos)
#define INTERVALO_ATUALIZACAO_PESO 500   // Intervalo de itens para atualizar o peso total

// Função para simular a esteira 1
void esteira1(int pipe_fd[]) {
    close(pipe_fd[0]); // Fecha a extremidade de leitura do pipe

    int contagem = 0;
    int peso_total = 0;
    while (1) {
        contagem++;
        peso_total = contagem * PESO_ESTEIRA1;
        write(pipe_fd[1], &contagem, sizeof(contagem));
        write(pipe_fd[1], &peso_total, sizeof(peso_total));

        usleep(INTERVALO_ESTEIRA1 * 1000000); // Espera em microssegundos
    }
}

// Função para simular a esteira 2
void esteira2(int pipe_fd[]) {
    close(pipe_fd[0]); // Fecha a extremidade de leitura do pipe

    int contagem = 0;
    int peso_total = 0;
    while (1) {
        contagem += 2;
        peso_total = contagem * PESO_ESTEIRA2;
        write(pipe_fd[1], &contagem, sizeof(contagem));
        write(pipe_fd[1], &peso_total, sizeof(peso_total));

        usleep(INTERVALO_ESTEIRA2 * 1000000); // Espera em microssegundos
    }
}

// Função para exibir informações sobre as esteiras
void exibicao(int pipe_fd1[], int pipe_fd2[]) {
    close(pipe_fd1[1]); // Fecha a extremidade de escrita do pipe da esteira 1
    close(pipe_fd2[1]); // Fecha a extremidade de escrita do pipe da esteira 2

    int contagem_total = 0;
    int ultimaContagemPeso = 0;
    int peso_total = 0;
    while (1) {
        int contagem1, peso1, contagem2, peso2;
        read(pipe_fd1[0], &contagem1, sizeof(contagem1));
        read(pipe_fd1[0], &peso1, sizeof(peso1));
        read(pipe_fd2[0], &contagem2, sizeof(contagem2));
        read(pipe_fd2[0], &peso2, sizeof(peso2));

        contagem_total = contagem1 + contagem2;

        if((ultimaContagemPeso + INTERVALO_ATUALIZACAO_PESO) <= contagem_total){
            peso_total = peso1 + peso2;
            ultimaContagemPeso = ultimaContagemPeso + INTERVALO_ATUALIZACAO_PESO;
        }
        
        

        printf("Esteira 1: Itens = %d, Peso = %d\n", contagem1, peso1);
        printf("Esteira 2: Itens = %d, Peso = %d\n", contagem2, peso2);
        printf("Total: Itens = %d, Peso = %d\n", contagem_total, peso_total);
        printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");

        sleep(INTERVALO_EXIBICAO); // Espera em segundos
    }
}

int main() {
    int pipe_fd_esteira1[2], pipe_fd_esteira2[2];

    if (pipe(pipe_fd_esteira1) == -1 || pipe(pipe_fd_esteira2) == -1) {
        perror("Erro ao criar pipes");
        exit(EXIT_FAILURE);
    }

    pid_t pid_esteira1 = fork();
    if (pid_esteira1 == 0) {
        close(pipe_fd_esteira1[0]); // Fecha a extremidade de leitura do pipe
        esteira1(pipe_fd_esteira1);
        exit(EXIT_SUCCESS);
    }

    pid_t pid_esteira2 = fork();
    if (pid_esteira2 == 0) {
        close(pipe_fd_esteira2[0]); // Fecha a extremidade de leitura do pipe
        esteira2(pipe_fd_esteira2);
        exit(EXIT_SUCCESS);
    }

    pid_t pid_exibicao = fork();
    if (pid_exibicao == 0) {
        exibicao(pipe_fd_esteira1, pipe_fd_esteira2);
        exit(EXIT_SUCCESS);
    }

    // Fechar as extremidades de escrita dos pipes principais no processo pai
    close(pipe_fd_esteira1[1]);
    close(pipe_fd_esteira2[1]);

    // Esperar os processos filhos terminarem
    waitpid(pid_esteira1, NULL, 0);
    waitpid(pid_esteira2, NULL, 0);
    waitpid(pid_exibicao, NULL, 0);

    return 0;
}
