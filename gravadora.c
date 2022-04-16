#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define PROD 1 //número de produtores
#define COMP 3 // número de compositores
#define CANT 5 // numeor de cantores
#define MUS 10 //tamanho do buffer da musica

#define COMPONDO 0          // estado da musica quando os compositores estao trabalhando nela
#define GRAVANDO (MUS/2)    // estado da musica quando os cantores estao trabahando nela
#define FINALIZANDO (MUS-1) // estado da musica quando o produtor precisa finaliza-la

int count = 0;                // contador de itens no buffer
int num_comp = 0;
int num_cant = 0;
int bufferMusica[MUS] = {0};    // buffer eh um array com tamanho MUS, iniciado com 0 em todas as posicoes
float estado = COMPONDO;        // estado atual da musica que esta sendo produzida
//int index_produtor = -1;      // index do produtor no buffer
//int index_consumidor = -1;    // index do consumidor no buffer

void * produtor(void * meuid);
void * compositor (void * meuid);
void * cantor (void * meuid);

void produz_musica();          // funcao para o produtor produzir um item
void progredir_musica(); // funcao para o produtor inserir o item no buffer
void produz_musica();          // funcao para o consumidor tirar um item do buffer
void compor_musica(int item);// funcao que simula o consumo do item pelo consumidor
void gravar_musica(int item);
void finalizar_musica();
void imprime_buffer();      // funcao que imprime o estado do buffer na tela

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;         // lock usado para as variaveis condicionais
pthread_mutex_t lock_estudio = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_comp = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_cant = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t produtor_cond = PTHREAD_COND_INITIALIZER;  // variavel condicional do produtor
pthread_cond_t compositor_cond = PTHREAD_COND_INITIALIZER;// variavel condicional do compositor
pthread_cond_t cantor_cond = PTHREAD_COND_INITIALIZER;    // variavel condicional do compositor

void main(argc, argv)
int argc;
char *argv[];
{

  int erro;
  int i, n, m;
  int *id;

  pthread_t tid[PROD];
   
  for (i = 0; i < PROD; i++)
  {
    id = (int *) malloc(sizeof(int));
    *id = i;
    erro = pthread_create(&tid[i], NULL, produtor, (void *) (id));

    if(erro)
    {
      printf("erro na criacao do thread %d\n", i);
      exit(1);
    }
  }

  pthread_t tCid[COMP];

  for (i = 0; i < COMP; i++)
  {
    id = (int *) malloc(sizeof(int));
    *id = i;
    erro = pthread_create(&tCid[i], NULL, compositor, (void *) (id));

    if(erro)
    {
      printf("erro na criacao do thread %d\n", i);
      exit(1);
    }
  }

  pthread_t tCaid[CANT];

  for (i = 0; i < CANT; i++)
  {
    id = (int *) malloc(sizeof(int));
    *id = i;
    erro = pthread_create(&tCaid[i], NULL, cantor, (void *) (id));

    if(erro)
    {
      printf("erro na criacao do thread %d\n", i);
      exit(1);
    }
  }
 
  pthread_join(tid[0],NULL);

}

void * produtor (void* pi){

  while(1){

    pthread_mutex_lock(&lock);                  // permite que soh o produtor acesse o buffer
    
        if(estado == COMPONDO){
            pthread_cond_broadcast(&compositor_cond);  // o produtor acorda o consumidor para voltar a consumir os itens
            printf("OS COMPOSITORES ESTÃO ACORDADOS\n");
        }

        if(estado == GRAVANDO){                           // se o buffer soh tiver um item
            pthread_cond_broadcast(&cantor_cond);  // o produtor acorda o consumidor para voltar a consumir os itens
            printf("OS CANTORES ESTÃO ACORDADOS\n");
        }

        if(estado == FINALIZANDO){
            finalizar_musica(); 
            count = 0;
            estado = COMPONDO;           
        }
    pthread_mutex_unlock(&lock);                // produtor libera o lock

    pthread_mutex_lock(&lock_estudio);
          
        produz_musica();                     
        progredir_musica();                      
    pthread_mutex_unlock(&lock_estudio);
  }
  pthread_exit(0);
  
}

void * compositor (void* pi){

  while(1){
    pthread_mutex_lock(&lock);                      // permite que soh o consumidor acesse o buffer
        
        while(estado != COMPONDO){                               // se o contador esta em 0, entao nao ha itens no buffer
            pthread_cond_wait(&compositor_cond, &lock); // entao o consumidor dorme
            printf("OS COMPOSITORES ESTÃO DORMINDO\n");
        }
    pthread_mutex_unlock(&lock);                    // consumidor libera o lock

    pthread_mutex_lock(&lock_comp);
        num_comp++;
        if(num_comp==1){
            pthread_mutex_lock(&lock_estudio);
        }
    pthread_mutex_unlock(&lock_comp);                 
        
        
        compor_musica(*(int *)(pi));                           // REGIAO CRITICA
        
    
    pthread_mutex_lock(&lock_comp);
        progredir_musica();
        num_comp--;
        if(num_comp==0){
            pthread_mutex_unlock(&lock_estudio);
        }
    pthread_mutex_unlock(&lock_comp);

  }
  pthread_exit(0);
  
}

void * cantor (void* pi){

  while(1){
    pthread_mutex_lock(&lock);                      // permite que soh o consumidor acesse o buffer
        while(estado != GRAVANDO){                               // se o contador esta em 0, entao nao ha itens no buffer
            pthread_cond_wait(&cantor_cond, &lock); // entao o consumidor dorme
            printf("OS CANTORES ESTÃO DORMINDO\n");
        }
    pthread_mutex_unlock(&lock);                    // consumidor libera o lock
        

    pthread_mutex_lock(&lock_cant);
        num_cant++;
        if(num_cant==1){
            pthread_mutex_lock(&lock_estudio);
        }
    pthread_mutex_unlock(&lock_cant);

        
        gravar_musica(*(int *)(pi));                           // consumidor consome o item retirado
        
    
    pthread_mutex_lock(&lock_cant);
        progredir_musica();
        num_cant--;
        if(num_cant==0){
            pthread_mutex_unlock(&lock_estudio);
        }
    pthread_mutex_unlock(&lock_cant);

  }
  pthread_exit(0);
  
}

void imprime_buffer(){          // funcao que imprime o estado do buffer na tela
  printf("ESTADO DO BUFFER: ");
  for(int i=0; i < MUS; i++){     // percorre todo o buffer e imprime cada item
    if(bufferMusica[i] == 1){
        printf("|X| ");
    }else{
        printf("| | ");
    }
  }
  printf("estado: %f count: %d\n", estado, count);
}

void produz_musica(){                                      // funcao que simula a producao de um item pelo produtor
  sleep(rand()%5);                                      // tempo aleatorio para simular o tempo de producao
  printf("O produtor produziu parte da música\n");
}

void progredir_musica(){             // funcao que insere o item produzido no buffer pelo produtor
    bufferMusica[count] = 1;        // o item eh colocado no buffer na posicao indicada pelo index
    count++;                                  // e aumenta o contador de itens no buffer
    if(count> 0 && count < MUS/2){
        estado = COMPONDO;
    }else if(count >= MUS/2 && count <= MUS-1){
        estado = GRAVANDO;
    }else{
        estado = FINALIZANDO;
    }
    imprime_buffer();                     // chama a funcao para mostrar o estado do buffer
}

void compor_musica(int comp){                    // funcao que simula o consumo do item pelo consumidor
  sleep(rand()%5);                              // tempo aleatorio para simular o tempo de consumo
  printf("O compositor %d compôs parte da música\n", comp);
}

void gravar_musica(int cant){                    // funcao que simula o consumo do item pelo consumidor
  sleep(rand()%5);                              // tempo aleatorio para simular o tempo de consumo
  printf("O cantor %d gravou parte da música\n", cant);
}

void finalizar_musica(){
    sleep(rand()%5);
    printf("A MÚSICA FOI FINALIZADA PELO PRODUTOR\n"); 
}