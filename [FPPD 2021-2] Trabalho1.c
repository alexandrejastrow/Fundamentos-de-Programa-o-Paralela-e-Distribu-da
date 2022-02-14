
#define _CRT_SECURE_NO_WARNIGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define VIRUS_MORTO 1
#define INJECAO 2
#define INSUMO_SECRETO 3

//objeto insumo
typedef struct insumo {
    int tpInsumo;
    int qtd;
    pthread_mutex_t* mutex;
    sem_t* signal;
}Insumo;

//objeto laboratorio
typedef struct lab {
    pthread_t id;
    int myId;
    int objetivo;
    int count;
    int* flag;
    sem_t* leave;
    Insumo* insumo1;
    Insumo* insumo2;
}Lab;


typedef struct infec {
    pthread_t id;
    int myId;
    int objetivo;
    int meuInsumo;
    int count;
    int* flag;
    sem_t* fila;
    sem_t* leave;
    Insumo* mesa_insumos;
}Infec;


//funcao thread infectado
void* infectado(void* arg) {

    Infec* f = (Infec*)arg;


    Insumo* insumo1, * insumo2;
    int i, flag = 1;
    while (1) {

        //variaveis auxiliares para representar os 2 insumos nescessarios
        insumo1 = NULL;
        insumo2 = NULL;

        //espero na fila para buscar meus insumos
        sem_wait(f->fila);

       //loop com a quantidade dde insumos ofertados "2 por laboratorio"
        for (i = 0; i < 6; i++) {

            
            if (insumo1 == NULL) {

                insumo1 = &(f->mesa_insumos[i]);

                //se o insumo atual for diferente do insumo exclusivo do infectado
                if (f->meuInsumo != insumo1->tpInsumo) {
                    
                    //travo esse insumo
                    pthread_mutex_lock(insumo1->mutex);
                    //se a quantidade for igual a zero, destravo esse insumo, e seto a var insumo1 para NULL
                    if (insumo1->qtd == 0) {
                        pthread_mutex_unlock(insumo1->mutex);
                        insumo1 = NULL;
                    }
                }
                else {
                    insumo1 = NULL;
                }
            }
            else {

                if (insumo2 == NULL) {

                    insumo2 = &(f->mesa_insumos[i]);
                    //verifica se o insumo atual é diferente do insumo exclusivo do infectado e se
                    // o insumo atual é diferente do insumo1 separado anteriormente
                    if (f->meuInsumo != insumo2->tpInsumo && insumo2->tpInsumo != insumo1->tpInsumo) {

                        //travo esse insumo
                        pthread_mutex_lock(insumo2->mutex);
                        //se a quantidade for igual a zero, destravo esse insumo, e seto a var insumo2 para NULL
                        if (insumo2->qtd == 0) {
                            pthread_mutex_unlock(insumo2->mutex);
                            insumo2 = NULL;
                        }

                    }
                    else {
                        insumo2 = NULL;
                    }
                }
                else {
                    //saio do loop FOR por ja ter conseguido os 2 insumos nescessarios
                    break;
                }

            }
        }

        //sinal para outra thread procurar os insumos nescessarios
        sem_post(f->fila);

        //verifica se a thread possui ous 2 insumos
        if (insumo1 && insumo2) {
            
            //decrementa 1 de cada insumo
            insumo1->qtd -= 1;
            insumo2->qtd -= 1;

            //verifica se a quantidade agora é zero, caso seja, manda um sinal para a thread dona do insumo
            //sinalizando que precisa fabricar mais
            if (insumo1->qtd == 0) {
                sem_post(insumo1->signal);
            }
            if (insumo2->qtd == 0) {
                sem_post(insumo2->signal);
            }
            //destrava os insumos
            pthread_mutex_unlock(insumo1->mutex);
            pthread_mutex_unlock(insumo2->mutex);

            //incrementa 1 na contagem do infectado
            f->count += 1;


            //verifica se o objetivo foi atingido
            if (f->count >= f->objetivo && flag) {
                flag = 0;
                //envia sinal para a maim
                sem_post(f->leave);
            }

            //verifica se a flag foi modificada para sair do programa
            printf("%d\n", *(f->flag));
            if (*(f->flag)) {
                printf("%d\n", *(f->flag));
                //passa por todos os insumos dando sinal para os laboratorios
                //caso contrario poderia ocorrer de um infectado sair sem dar um sinal para um laboratorio
                //deixando o mesmo travado no semaphoro
                for (i = 0; i < 6; i++) {
                    sem_post(f->mesa_insumos[i].signal);

                }
                return NULL;
            }
        }
        else {

            //caso chegue aqui, nao foi possivel pegar os 2 insumos na busca

            //verifica se tem algum insumo, e destrava se tiver
            if (insumo1) {
                pthread_mutex_unlock(insumo1->mutex);
            }
            if (insumo2) {
                pthread_mutex_unlock(insumo2->mutex);
            }
        }
    }

    return NULL;
}

//funcao thread laboratorio
void* laboratorio(void* arg) {

    Lab* l = (Lab*)arg;

    int qtd_insumo_fabricado;
    int flag = 1;
    while (1) {

        //gera um valor randomico de insumo
        qtd_insumo_fabricado = (rand() % 10) + 1;


        //espera um sinal de um infectado dizendo que este insumo acabou
        sem_wait(l->insumo1->signal);
        
        //bloqueia o insumo e "fabrica" uma nova quantidade
        pthread_mutex_lock(l->insumo1->mutex);
        l->insumo1->qtd += qtd_insumo_fabricado;
        pthread_mutex_unlock(l->insumo1->mutex);


        //mesmo processo do insumo1 repetido no insumo2
        sem_wait(l->insumo2->signal);
        
        pthread_mutex_lock(l->insumo2->mutex);
        l->insumo2->qtd += qtd_insumo_fabricado;
        pthread_mutex_unlock(l->insumo2->mutex);

        //adiciona no contador do laboratorio a quantidade fabricada nessa rodada
        l->count += qtd_insumo_fabricado;

        //verifica se o objetivo foi atingido
        if (l->count >= l->objetivo && flag) {
            flag = 0;
            //envia sinal para a maim
            sem_post(l->leave);
        }

        //verifica se a flag foi modificada para sair do programa
        if (*(l->flag)) {
            return NULL;
        }

    }
}

int main(int argc, char* argv[])
{

    int objetivo;

    if (argc != 2) {
        return  -1;
    }
    else {
        objetivo = atoi(argv[1]);
    }

    srand(time(NULL));

    //declaração das variaveis
    Insumo* insumos = (Insumo*)malloc(sizeof(Insumo) * 6);

    Lab* laboratorios = (Lab*)malloc(sizeof(Lab) * 3);

    Infec* infectados = (Infec*)malloc(sizeof(Infec) * 3);

    int i, flags[6];
    sem_t fila, signal[6], leave;
    pthread_mutex_t mutex[6];

    
    sem_init(&fila, 0, 1);
    sem_init(&leave, 0, 0);

    for (i = 0; i < 6; i++) {
        pthread_mutex_init(&mutex[i], NULL);
        sem_init(&signal[i], 0, 1);

        insumos[i].mutex = &mutex[i];
        insumos[i].qtd = 0;
        insumos[i].signal = &signal[i];

        flags[i] = 0;
    }

    //definindo os tipos de insumo
    insumos[0].tpInsumo = VIRUS_MORTO;
    insumos[1].tpInsumo = VIRUS_MORTO;
    insumos[2].tpInsumo = INJECAO;
    insumos[3].tpInsumo = INJECAO;
    insumos[4].tpInsumo = INSUMO_SECRETO;
    insumos[5].tpInsumo = INSUMO_SECRETO;


    //atribuindo 2 insumos diferentes para cada laboratorio
    laboratorios[0].insumo1 = &insumos[0];
    laboratorios[0].insumo2 = &insumos[3];


    laboratorios[1].insumo1 = &insumos[1];
    laboratorios[1].insumo2 = &insumos[4];


    laboratorios[2].insumo1 = &insumos[2];
    laboratorios[2].insumo2 = &insumos[5];

    //definindo o insumo de cada infectado
    infectados[0].meuInsumo = VIRUS_MORTO;
    infectados[1].meuInsumo = INJECAO;
    infectados[2].meuInsumo = INSUMO_SECRETO;

    //atribuindo os valores para os laboratorios e iniciando as threads
    for (i = 0; i < 3; i++) {
        laboratorios[i].count = 0;
        laboratorios[i].flag = &flags[i];
        laboratorios[i].myId = i;
        laboratorios[i].objetivo = objetivo;
        laboratorios[i].leave = &leave;
        pthread_create(&laboratorios[i].id, NULL, laboratorio, (void*)&laboratorios[i]);
    }

    //atribuindo os valores para os infectados e iniciando as threads
    for (i = 0; i < 3; i++) {

        infectados[i].myId = i;
        infectados[i].mesa_insumos = insumos;
        infectados[i].objetivo = objetivo;
        infectados[i].fila = &fila;
        infectados[i].leave = &leave;
        infectados[i].flag = &flags[i + 3];
        infectados[i].count = 0;
        pthread_create(&infectados[i].id, NULL, infectado, (void*)&infectados[i]);
    }

    //espera os sinais das threads indicando que conseguiu atingir o objetivo
    for (i = 0; i < 6; i++) {
        sem_wait(&leave);
    }
    //seta as flags para 1 dizendo para as threads que elas podem finalizar o serviço
    for (i = 0; i < 6; i++) {
        flags[i] = 1;
    }

    //espera as threads finalizarem
    for (i = 0; i < 3; i++) {
        pthread_join(infectados[i].id, NULL);
        printf("infectado %d: %d\n", infectados[i].myId, infectados[i].count);
    }
    for (i = 0; i < 3; i++) {
        pthread_join(laboratorios[i].id, NULL);
        printf("laboratorio %d: %d\n", laboratorios[i].myId, laboratorios[i].count);
    }
    return 0;
}
