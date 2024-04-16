#include <stdio.h>
#include <pthread.h>

// Define constants
#define CONVEYOR1_WEIGHT 5
#define CONVEYOR2_WEIGHT 2
#define CONVEYOR1_INTERVAL 2
#define CONVEYOR2_INTERVAL 1
#define DISPLAY_INTERVAL 2         // Intervalo de tempo para atualizaÃ§Ã£o da exibiÃ§Ã£o (em segundos)
#define UPDATE_WEIGHT_INTERVAL 500 // Intervalo de itens para atualizar o peso total

// Define data structures
typedef struct {
    int weight;
    int count;
    int total_weight;
} Conveyor;

// Define global variables
Conveyor conveyor1 = {CONVEYOR1_WEIGHT, 0, 0};
Conveyor conveyor2 = {CONVEYOR2_WEIGHT, 0, 0};
pthread_mutex_t conveyor_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para exclusÃ£o mÃºtua
int lastCount = 0;

// FunÃ§Ã£o para bloquear o mutex
void lock_mutex() {
    pthread_mutex_lock(&conveyor_mutex);
}

// FunÃ§Ã£o para desbloquear o mutex
void unlock_mutex() {
    pthread_mutex_unlock(&conveyor_mutex);
}

// FunÃ§Ã£o para simular a esteira 1
void *conveyor1_thread(void *arg) {
    while (1) {
        lock_mutex();
        conveyor1.count++;
        conveyor1.total_weight += CONVEYOR1_WEIGHT;
        unlock_mutex();

        sleep(CONVEYOR1_INTERVAL);
    }
}

// FunÃ§Ã£o para simular a esteira 2
void *conveyor2_thread(void *arg) {
    while (1) {
        lock_mutex();
        conveyor2.count++;
        conveyor2.total_weight += CONVEYOR2_WEIGHT;
        unlock_mutex();

        sleep(CONVEYOR2_INTERVAL);
    }
}

// FunÃ§Ã£o para exibir informaÃ§Ãµes sobre as esteiras
void *display_thread(void *arg) {
    int total_count = 0;  // NÃºmero total de itens em ambas as esteiras
    int total_weight = 0; // Peso total transportado por ambas as esteiras
    while (1) {
        lock_mutex(); // Bloqueia o mutex para acesso seguro Ã s variÃ¡veis compartilhadas
        total_count = conveyor1.count + conveyor2.count;

        if((lastCount + 500) < total_count){
            total_weight = conveyor1.total_weight + conveyor2.total_weight;
            lastCount = lastCount + 500;
        }
        
        unlock_mutex(); // Libera o mutex apÃ³s o acesso Ã s variÃ¡veis compartilhadas

        // Exibe as informaÃ§Ãµes sobre as esteiras e o peso total
        printf("Conveyor 1: Items = %d, Weight = %d\n", conveyor1.count, conveyor1.total_weight);
        printf("Conveyor 2: Items = %d, Weight = %d\n", conveyor2.count, conveyor2.total_weight);
        printf("Total: Items = %d, Weight = %d\n", total_count, total_weight);

        // Verifica se Ã© hora de atualizar o peso total e exibe uma mensagem de atualizaÃ§Ã£o
        if (total_count % UPDATE_WEIGHT_INTERVAL == 0) {
            printf("Weight updated: %d\n", total_weight);
        }

        printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
        sleep(DISPLAY_INTERVAL);
    }
}

int main() {
    pthread_t conveyor1_tid, conveyor2_tid, display_tid;

    // Cria threads para as esteiras e para exibir informaÃ§Ãµes
    pthread_create(&conveyor1_tid, NULL, conveyor1_thread, NULL);
    pthread_create(&conveyor2_tid, NULL, conveyor2_thread, NULL);
    pthread_create(&display_tid, NULL, display_thread, NULL);

    // Aguarda o tÃ©rmino das threads
    pthread_join(conveyor1_tid, NULL);
    pthread_join(conveyor2_tid, NULL);
    pthread_join(display_tid, NULL);

    return 0;
}