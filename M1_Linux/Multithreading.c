#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h> // Para utilizar a função select()

// Define constantes
#define PESO_ESTEIRA1 5
#define PESO_ESTEIRA2 2
#define INTERVALO_ESTEIRA1 2
#define INTERVALO_ESTEIRA2 1
#define INTERVALO_EXIBICAO 2             // Intervalo de tempo para atualização da exibição (em segundos)
#define INTERVALO_ATUALIZACAO_PESO 500   // Intervalo de itens para atualizar o peso total

// Define estruturas de dados
typedef struct {
    int peso;
    int contagem;
    int peso_total;
} Esteira;

// Define variáveis globais
Esteira esteira1 = {PESO_ESTEIRA1, 0, 0};
Esteira esteira2 = {PESO_ESTEIRA2, 0, 0};
pthread_mutex_t mutex_esteira = PTHREAD_MUTEX_INITIALIZER; // Mutex para exclusão mútua
int ultimaContagem = 0;

// Função para bloquear o mutex
void bloquear_mutex() {
    pthread_mutex_lock(&mutex_esteira);
}

// Função para desbloquear o mutex
void desbloquear_mutex() {
    pthread_mutex_unlock(&mutex_esteira);
}

// Função para verificar se há entrada de teclado
int kbhit(void) {
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); // STDIN_FILENO é o descritor de arquivo para entrada padrão (teclado)
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

void *thread_esteira1(void *arg) {
    while (1) {
        bloquear_mutex();
        esteira1.contagem++;
        esteira1.peso_total += PESO_ESTEIRA1;
        desbloquear_mutex();

        sleep(INTERVALO_ESTEIRA1);

        // Verifica se há entrada de teclado
        if (kbhit()) {
            printf("\nSistema pausado pelo usuário.\n");
            break;
        }
    }
    return NULL;
}

void *thread_esteira2(void *arg) {
    while (1) {
        bloquear_mutex();
        esteira2.contagem++;
        esteira2.peso_total += PESO_ESTEIRA2;
        desbloquear_mutex();

        sleep(INTERVALO_ESTEIRA2);

        // Verifica se há entrada de teclado
        if (kbhit()) {
            printf("\nSistema pausado pelo usuário.\n");
            break;
        }
    }
    return NULL;
}

// Função para exibir informações sobre as esteiras
void *thread_exibicao(void *arg) {
    int contagem_total = 0;  // Número total de itens em ambas as esteiras
    int peso_total = 0;      // Peso total transportado por ambas as esteiras
    while (1) {
        bloquear_mutex(); // Bloqueia o mutex para acesso seguro às variáveis compartilhadas
        contagem_total = esteira1.contagem + esteira2.contagem;

        if((ultimaContagem + 500) < contagem_total){
            peso_total = esteira1.peso_total + esteira2.peso_total;
            ultimaContagem = ultimaContagem + 500;
        }
        
        desbloquear_mutex(); // Libera o mutex após o acesso às variáveis compartilhadas

        // Exibe as informações sobre as esteiras e o peso total
        printf("Esteira 1: Itens = %d, Peso = %d\n", esteira1.contagem, esteira1.peso_total);
        printf("Esteira 2: Itens = %d, Peso = %d\n", esteira2.contagem, esteira2.peso_total);
        printf("Total: Itens = %d, Peso = %d\n", contagem_total, peso_total);

        printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
        sleep(INTERVALO_EXIBICAO);

        // Verifica se há entrada de teclado
        if (kbhit()) {
            printf("\nSistema pausado pelo usuário.\n");
            break;
        }
    }
    return NULL;
}

int main() {
    pthread_t tid_esteira1, tid_esteira2, tid_exibicao;

    // Cria threads para as esteiras e para exibir informações
    pthread_create(&tid_esteira1, NULL, thread_esteira1, NULL);
    pthread_create(&tid_esteira2, NULL, thread_esteira2, NULL);
    pthread_create(&tid_exibicao, NULL, thread_exibicao, NULL);

    // Aguarda o término das threads
    pthread_join(tid_esteira1, NULL);
    pthread_join(tid_esteira2, NULL);
    pthread_join(tid_exibicao, NULL);

    return 0;
}
