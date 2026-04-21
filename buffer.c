#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

//
// Definição dos semáforos
//
sem_t mutex;
sem_t empty;
sem_t full;

// ponteiro para a fila do buffer
int * buffer;

// buffer size
int N_BUFFER;

// producers num
int PROD_NUM;

// posicao do buffer a gravar
int p = 0;

// posicoes para acessar o buffer
int in = 0;
int out = 0;

// prototipos das funcoes
void *producer(void *);
void *consumer(void *);
int gera_rand(int);
void print_buffer();

int main(int argc, char ** argv)
{
    // thread do consumidor
    pthread_t tcons;

    // threads dos n produtores (vao ser alocadas dinamicamente)
    pthread_t *nprod;

    long i;

    if ( argc != 3 )
    {
        printf("Usage: %s buffer_size num_producers\n", argv[0]);
        return 0;
    }

    // iniciando a semente do random
    srand(time(NULL));

    // buffer size
    N_BUFFER = atoi(argv[1]);

    // num of producers
    PROD_NUM = atoi(argv[2]);

    //
    // Criação dos semáforos
    //
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, N_BUFFER);
    sem_init(&full, 0, 0);
    
    // gerando um buffer de N inteiros
    buffer = malloc(N_BUFFER * sizeof(int));

    // "zerando" o buffer com valores -1
    for (i = 0; i < N_BUFFER; i++)
    {
        buffer[i] = -1;
    }

    // gerando uma lista de threads de produtores
    nprod = malloc(PROD_NUM * sizeof(pthread_t));

    // iniciando a thread do consumidor
    pthread_create(&tcons, NULL, consumer, NULL);

    // iniciando as threads dos produtores
    for (i = 0; i < PROD_NUM; i++)
    {
        pthread_create(&nprod[i], NULL, producer, (void *)i);
    }
    
    // aguardando as threads dos produtores
    for (i = 0; i < PROD_NUM; i++)
    {
        pthread_join(nprod[i], NULL);
    }

    // finalizando a thread do consumidor
    pthread_join(tcons, NULL);

    //
    // Excluindo os semáforos
    //
    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);

    // liberando a memoria alocada
    free(buffer);
    free(nprod);

    return 0;
}

void * consumer(void *arg)
{
    usleep(gera_rand(1000000));

    // o valor consumido será salvo nesta variável
    int produto;

    int i = 0;

    // consome todos os recursos dos n produtores e finaliza
    while (i != PROD_NUM)
    {
        printf("- Consumidor esperando por recurso!\n");

        print_buffer();

        //
        // bloqueia até que tenha algo a consumir
        //
        sem_wait(&full);
        
        printf("- Consumidor entrou em ação!\n");

        print_buffer();

        //
        // garante exclusão mutua ao acessar o buffer
        //
        sem_wait(&mutex);

        printf("\t- Consumidor vai limpar posição %d\n", out);

        // acessando o buffer
        produto = buffer[out];
        printf("\t- Consumiu o valor: %d\n", produto);

        // verificando se o consumidor nao esta consumindo sujeira do buffer
        if (buffer[out] == -1)
        {
            printf("==== ALERTA DO CONSUMIDOR ====\n");
            printf("Posicao %d estava vazia\n", out);
            printf("==============================\n");
        }
        
        // limpando o buffer nesta posição
        buffer[out] = -1;
        out = (out + 1) % N_BUFFER;

        //
        // saindo da seção critica
        //
        sem_post(&mutex);
        
        //
        // liberando uma posição vazia para produtor
        //
        sem_post(&empty);

        i++;
    }

    return NULL;
}

// recebe o Id de um produtor
void * producer(void * id)
{
    usleep(gera_rand(1000000));

    long i = (long)id;
    
    // valor a ser produzido
    int produto;

    printf("> Produtor %ld esperando por recurso!\n", i);

    //
    // bloqueia até que tenha posicao disponível no buffer
    //
    sem_wait(&empty);

    printf("> Produtor %ld entrou em ação!\n", i);

    // 
    // garante o acesso exclusivo ao buffer
    //
    sem_wait(&mutex);

    // numero aleatorio de 0 a 99
    produto = gera_rand(100);

    // verificando se o produtor nao esta sobrescrevendo uma posicao
    if (buffer[in] != -1)
    {
        printf("==== ALERTA DO PRODUTOR %ld ====\n", i);
        printf("Posicao %d ocupada com o valor %d\n", in, buffer[in]);
        printf("===============================\n");
    }

    // gravando no buffer
    printf("\t> Produtor %ld vai gravar o valor %d na pos %d\n", i, produto, in);
    buffer[in] = produto;
    
    // avancando a posicao do buffer
    in = (in + 1) % N_BUFFER;

    // 
    // libera o acesso ao buffer
    //
    sem_post(&mutex);
    
    //
    // libera para o consumidor acessar o buffer
    //
    sem_post(&full);

    return NULL;
}

int gera_rand(int limit)
{
    // 0 a (limit - 1)
    return rand() % limit;
}

void print_buffer()
{
    int i;
    printf("\t== BUFFER ==\n");
    for (i = 0; i < N_BUFFER; i++)
    {
        printf("\ti: %d | v: %d\n", i, buffer[i]);
    }
    printf("\n");
}