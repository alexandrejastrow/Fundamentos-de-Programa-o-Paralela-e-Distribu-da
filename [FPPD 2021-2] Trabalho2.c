
#define _CRT_SECURE_NO_WARNIGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1


#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#define TRUE 1
#define FALSE 0

struct client {
    pthread_t id;
    int myId;
    int* idBarbeiro;
    sem_t* sem_fila;
    sem_t* sem_cad_espera;
    sem_t* sem_cad_client;
    sem_t* sem_cad_barbeiro;
    sem_t* sem_sinal_cabelo_cortado;
    sem_t* signal;
    pthread_mutex_t* mutex;

};

struct barber {
    pthread_t id;
    int myId;
    int* idBarbeiro;
    int* contador_parada;
    int numeroMinimoDeClientes;
    int count;
    pthread_mutex_t* mutex;
    pthread_mutex_t* mutex_parada;
    sem_t* sem_fila;
    sem_t* sem_cads_client;
    sem_t* sem_sinal_cabelo_cortado;
    sem_t* signal;
};


typedef struct client Client;
typedef struct barber Barber;


void* BarbeiroFunc(void* arg) {

   
    Barber* bab = (Barber*)arg;

    int flag = 1;
    while (TRUE) {


        //ordena a entrada dos barbeiros
        sem_wait(bab->sem_fila);


        pthread_mutex_lock(bab->mutex);
        //insere sua ID para um cliente indentifica-lo
        *(bab->idBarbeiro) = bab->myId;

        pthread_mutex_unlock(bab->mutex);

        //envia sinal para o cliente indicando que a variavel compartilhada do ID foi preenchida
        sem_post(bab->signal);

        //espera um sinal do cliente
        sem_wait(&(bab->sem_cads_client[bab->myId]));

        //tempo do corte de cabelo   -> funciona sem, porem demora mais e alguns barbeiros trabalham bem mais que os outros
        Sleep(5);

        //"corta o cabelo do cliente" envia um sinal
        sem_post(bab->sem_sinal_cabelo_cortado);
        
        //adiciona 1 na contagem
        bab->count += 1;

        //verifica se atingiu a meta
        if (bab->count >= bab->numeroMinimoDeClientes && flag) {
            pthread_mutex_lock(bab->mutex_parada);
            //adciona 1 no contador de barbeiros que atingiram seu objetivo
            *(bab->contador_parada) += 1;

            pthread_mutex_unlock(bab->mutex_parada);
            flag = 0;
        
        }
    }
}

void* clientFunction(void* arg) {

    Client* cli = (Client*)arg;



    // tenta pegar uma das cadeiras de espera
    if (sem_trywait(cli->sem_cad_espera) == 0) {


        int idBarbeiro;

        //espera um dinal do barbeiro, isso indica que um deles inseriu sua ID na variavel cmpartilhada
        sem_wait(cli->signal);

        pthread_mutex_lock(cli->mutex);
        //pega a ID do barbeiro disponivel
        idBarbeiro = *(cli->idBarbeiro);


        pthread_mutex_unlock(cli->mutex);
        
        //sinal para o proximo barbeiro
        sem_post(cli->sem_fila);

        //espera uma cadeira de barbeiro ficar vaga
        sem_wait(&(cli->sem_cad_barbeiro[idBarbeiro]));
        
        //libera a cadeira do cliente
        sem_post(&(cli->sem_cad_client[idBarbeiro]));

        //libera uma cadeira de espera
        sem_post(cli->sem_cad_espera);

        //espera um sinal indicando que o cabelo foi cortado
        sem_wait(&(cli->sem_sinal_cabelo_cortado[idBarbeiro]));

        //libera a cadeira do barbeiro
        sem_post(&(cli->sem_cad_barbeiro[idBarbeiro]));

    }


     //limpo a memoria deste cliente
     free(cli);
     return NULL;
}


int main(int argc, char* argv[]) {


    int qtdBarbeiros, qtdCadeiras, qtdMinimaClientes, i;
    

    if (argc == 4) {
        qtdBarbeiros = atoll(argv[1]);
        qtdCadeiras = atoll(argv[2]);
        qtdMinimaClientes = atoll(argv[3]);

        if (qtdBarbeiros <= 0 || qtdCadeiras <= 0 || qtdMinimaClientes <= 0) {
            return -1;
        }
    }
    else {
        return -1;
    }

    /*
    qtdBarbeiros = 50;
    qtdCadeiras = 2;
    qtdMinimaClientes = 50;
    */
    

    //variaveis compartilhadas
    int idBarberiroCompartilhada, contador_parada;

    idBarberiroCompartilhada = 0;
    contador_parada = 0;


    Barber* barbeiros = malloc(sizeof(Barber) * qtdBarbeiros);
    
    //semaphoro das cadeiras de espera
    sem_t sem_cadeira_espera;
    sem_init(&sem_cadeira_espera, 0, qtdCadeiras);

    //semaphoro representando as cadeiras dos barbeiros
    sem_t* sem_cads_barbeiro = malloc(sizeof(sem_t) * qtdBarbeiros);

    //semaphoros de controlo indicando que o cabelo foi cortado
    sem_t* sem_sinal_cabelo_cortado = malloc(sizeof(sem_t) * qtdBarbeiros);

    //semaphoro representando as cadeiras dos clientes
    sem_t* sem_cads_client = malloc(sizeof(sem_t) * qtdBarbeiros);

    //semaphoro da um sinal dizendo que tem um barbeiro disponivel
    sem_t signal;
    sem_init(&signal, 0, 0);

    //semaphoro que controla a ordem dos barbeiros
    sem_t sem_fila;
    sem_init(&sem_fila, 0, 1);

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    
    pthread_mutex_t mutex_parada;
    pthread_mutex_init(&mutex_parada, NULL);

    for (i = 0; i < qtdBarbeiros; i++) {
        
        sem_init(&(sem_cads_barbeiro[i]), 0, 1);
        sem_init(&(sem_sinal_cabelo_cortado[i]), 0, 0);
        sem_init(&(sem_cads_client[i]), 0, 0);
    }

    for (i = 0; i < qtdBarbeiros; i++) {

        barbeiros[i].myId = i;
        barbeiros[i].idBarbeiro = &idBarberiroCompartilhada;
        barbeiros[i].count = 0;
        barbeiros[i].mutex = &mutex;
        barbeiros[i].sem_fila = &sem_fila;
        barbeiros[i].sem_sinal_cabelo_cortado = &(sem_sinal_cabelo_cortado[i]);
        barbeiros[i].numeroMinimoDeClientes = qtdMinimaClientes;
        barbeiros[i].sem_cads_client = sem_cads_client;
        barbeiros[i].signal = &signal;
        barbeiros[i].contador_parada = &contador_parada;
        barbeiros[i].mutex_parada = &mutex_parada;

        pthread_create(&(barbeiros[i].id), NULL, BarbeiroFunc, &(barbeiros[i]));
    }

    int idCounter = 0;

    //loop cria varios clientes que disputaram pelas cadeiras de espera
    while (TRUE) {

        Client* client = (Client*)malloc(sizeof(Client));

        client->myId = idCounter;
        client->idBarbeiro = &idBarberiroCompartilhada;
        client->mutex = &mutex;
        client->sem_fila = &sem_fila;
        client->sem_cad_client = sem_cads_client;
        client->sem_cad_barbeiro = sem_cads_barbeiro;
        client->sem_sinal_cabelo_cortado = sem_sinal_cabelo_cortado;
        client->sem_cad_espera = &sem_cadeira_espera;
        client->signal = &signal;

        pthread_create(&(client->id), NULL, clientFunction, client);
        idCounter++;

        pthread_mutex_lock(&mutex_parada);
        //condicional que verifica se todos os barbeiros atingiram o objetivo minimo
        if (contador_parada >= qtdBarbeiros) break;

        pthread_mutex_unlock(&mutex_parada);
    }
    //imprime o resultado
    for (i = 0; i < qtdBarbeiros; i++)
    {
        printf("barbeiro %d atendeu %d clientes\n", barbeiros[i].myId, barbeiros[i].count);

        pthread_cancel(barbeiros[i].id);
    }
    return 0;
}