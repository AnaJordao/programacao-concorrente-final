#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define PROD 1 //número de produtores
#define COMP 3 // número de compositores
#define CANT 5 // numero de cantores
#define MUS 10 //tamanho do buffer da musica
#define META 3 // meta que o produtor tem que bater para ter 1 dia de folga

#define COMPONDO 0          // estado da musica quando os compositores estao trabalhando nela
#define GRAVANDO (MUS/2)    // estado da musica quando os cantores estao trabahando nela
#define FINALIZANDO (MUS-1) // estado da musica quando o produtor precisa finaliza-la

int count = 0;                // contador de itens no buffer (barra de prograsso para o termino da musica)
int quantidade_musicas = 0;   // quantidade de musicas prontas
int bufferMusica[MUS] = {0};  // buffer eh um array com tamanho MUS, iniciado com 0 em todas as posicoes
float estado = COMPONDO;      // estado inicial da musica que esta sendo produzida

void * produtor(void * meuid);    // funcao do produtor
void * compositor (void * meuid); // funcao do compositor
void * cantor (void * meuid);     // funcao do cantor

void produz_musica();         // funcao que simula o produtor produzindo uma parte da musica
void progredir_musica();      // funcao para que a barra de progresso (buffer) seja atualizada
void compor_musica(int item); // funcao que simula o compositor compondo uma parte da musica
void gravar_musica(int item); // funcao que simula o cantor gravando uma parte da musica
void finalizar_musica();      // funcao que simula o produtor finalizando a musica
void imprime_buffer();        // funcao que imprime o estado da barra de progresso (buffer) na tela
void limpa_buffer();          // funcao que limpa o buffer quando uma nova musica comeca a ser feita

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;         // lock usado para as variaveis condicionais
pthread_mutex_t lock_estudio = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_folga = PTHREAD_MUTEX_INITIALIZER;   // lock usado para quando o produtor tira folga
pthread_cond_t produtor_cond = PTHREAD_COND_INITIALIZER;  // variavel condicional do produtor
pthread_cond_t compositor_cond = PTHREAD_COND_INITIALIZER;// variavel condicional do compositor
pthread_cond_t cantor_cond = PTHREAD_COND_INITIALIZER;    // variavel condicional do compositor
sem_t sem_musicas;                                        // semaforo que serve de contador de musicas ja feitas

void main(argc, argv) // na funcao main, o semaforo sera inicializado e as threads serao criadas
int argc;
char *argv[];
{

  int erro;
  int i, n, m;
  int *id;

  sem_init(&sem_musicas, 0, 0); // eh inicializado com 0 permissoes pois nenhuma musica foi feita ainda

  pthread_t tid[PROD];  // criacao de 1 produtor
   
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

  pthread_t tCid[COMP]; // criacao de um numero COMP de compositores

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

  pthread_t tCaid[CANT];  // criacao de um numero CANT de cantores

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

void * produtor (void* pi){   // o produtor pode trabalhar em qualquer estado da musica e somente ele pode finaliza-la

  while(1){
    pthread_mutex_lock(&lock);                                      // o produto pega o lock             
    
      if(estado == COMPONDO || (count>= 0 && count < MUS/2)){       // se o estado da musica for COMPONDO
        pthread_cond_broadcast(&compositor_cond);                   // os compositores acordam
      }

      if(estado == GRAVANDO || (count >= MUS/2 && count < MUS-1)){  // se o estado da musica for GRAVANDO                  
        pthread_cond_broadcast(&cantor_cond);                       // os cantores acordam
      }

      if(estado == FINALIZANDO || count == MUS - 1){  // se o estado da musica for FINALIZANDO
        quantidade_musicas++;                         // o contador de musicad prontas eh incrementado
        finalizar_musica();                           // chama a funcao para simular a finalizacao da musica
        progredir_musica();                           // chama a funcao que atualiza o buffer e o estado
        count = 0;                                    // o contador do buffer retorna a 0
        limpa_buffer();                               // o buffer eh limpado para comecar uma nova musica
        estado = COMPONDO;                            // o estado volta ao inicial COMPONDO
          
        sem_post(&sem_musicas);                       // da um post no semaforo de musicas feitas
        pthread_cond_broadcast(&compositor_cond);     // acorda os compositores novamente
      }
    pthread_mutex_unlock(&lock);                      // devolve o lock 

    pthread_mutex_lock(&lock_folga);        // o produtor pega o lock a folga
      if(quantidade_musicas != META){       // se o contador de musicas prontas for diferente da meta, entao ele ainda nao a bateu
        produz_musica();                    // chama a funcao para simular a producao de parte da musica          
        progredir_musica();                 // chama a funcao para progredir a barra de progresso
      }         
      else if(quantidade_musicas == META){  // se a quantidade de musicas prontas for igual a meta, entao ele bate
        printf("O PRODUTOR BATEU A META E TIROU UM DIA DE FOLGA!\n");
        sleep(30);                          // tempo para simular a folga do produtor

        for(int i = META; i > 0; i--){      // ele volta todas as permissoes do semaforo   
            sem_wait(&sem_musicas); 
        }
        quantidade_musicas = 0;             // a quantidade de musicas prontas volta a ser 0
        printf("O PRODUTOR VOLTOU DA FOLGA!\n");// ele volta da folga
      }
    pthread_mutex_unlock(&lock_folga);      // devolve o lock
  }
  pthread_exit(0);
}

void * compositor (void* pi){ // o compositor soh pode trabalhar na musica quando o estado dela eh COMPONDO

  while(1){  
    pthread_mutex_lock(&lock);                                    // compositor pega o lock
      while(estado != COMPONDO || !(count>= 0 && count < MUS/2)){ // se o estado nao estiver em COMPONDO                          
          pthread_cond_wait(&compositor_cond, &lock);             // entao ele dorme
      }
 
      compor_musica(*(int *)(pi));                                // se nao estiver dormindo ele compoe parte da musica
      progredir_musica();                                         // chama a funcao para progredir a barra de progresso

      if(estado == GRAVANDO || (count >= MUS/2 && count < MUS-1)){// se o estado se tornar GRAVANDO
        pthread_cond_broadcast(&cantor_cond);                     // acorda os cantores
      }
    pthread_mutex_unlock(&lock);                                  // devolve o lock
    sleep(3);                                                     // sleep somente para mostrar uma diferenciacao nos compositores
  }
  pthread_exit(0);
}

void * cantor (void* pi){     // o cantor soh pode trabalhar na musica quando o estado dela eh GRAVANDO

  while(1){
    pthread_mutex_lock(&lock);                                        // cantor pega o lock
      while(estado != GRAVANDO || !(count >= MUS/2 && count < MUS-1)){// se o estado for diferente de GRAVANDO
        pthread_cond_wait(&cantor_cond, &lock);                       // entao os cantoes dormem
      }
      
      gravar_musica(*(int *)(pi));                                    // se estiverem acordados eles gravam parte da musica
      progredir_musica();                                             // chama a funcao para progredir a barra de progresso

      if(estado == FINALIZANDO || count == MUS - 1){                  // se o estado mudar para FINALIZANDO
        pthread_cond_wait(&cantor_cond, &lock);                       // os cantores dormem
      }
    pthread_mutex_unlock(&lock);                                      // devolve o lock   
    sleep(3);                                                         // sleep somente para mostrar uma diferenciacao nos cantores
  }
  pthread_exit(0);
  
}

void imprime_buffer(){          // funcao para imprimir o buffer
  printf("ESTADO DO BUFFER: ");
  for(int i=0; i < MUS; i++){     
    if(bufferMusica[i] == 1){   // se o index for 1, quer dizer vai ser preenchido
        printf("|X| ");
    }else{                      // senao ficara vazio
        printf("|_| ");
    }
  }
  printf("\n");
}

void limpa_buffer(){          // funcao para limpar o buffer quando a musica termina de ser feita e uma nova começa
  for(int i=0; i < MUS; i++){     
    bufferMusica[i] = 0;      // substitui todos os numeros do array por 0, ou seja, a barra de progreso fica vazia novamente
  }
}

void produz_musica(){ // funcao para simular a producao da musica                         
  sleep(rand()%5);    // tempo de simulacao para a producao                            
  printf("O produtor produziu parte da música\n");
}

void progredir_musica(){            // funcao para atualizar a barra de progresso (buffer) 
    bufferMusica[count] = 1;        // a posicao usando o contador como indice sera preenchido com 1
    count++;                        // incrementa o contador    
    if(count> 0 && count < MUS/2){  // atualiza o estado com base no count
        estado = COMPONDO;
    }else if(count >= MUS/2 && count < MUS-1){
        estado = GRAVANDO;
    }else{
        estado = FINALIZANDO;
    }
    imprime_buffer();               // chama a funcao para imprimir o buffer na tela    
}

void compor_musica(int comp){ // funcao para simular a composicao da musica                 
  sleep(rand()%5);            // tempo de simulacao para a composicao                  
  printf("O compositor %d compôs parte da música\n", comp);
}

void gravar_musica(int cant){ // funcao para simular a gravacao da musica                   
  sleep(rand()%5);            // tempo de simulacao para a gravacao                  
  printf("O cantor %d gravou parte da música\n", cant);
}

void finalizar_musica(){  // funcao para simular a finalizacao da musica pelo produtor
    sleep(rand()%5);      // tempo de simulacao para a finalizacao
    printf("A MÚSICA FOI FINALIZADA PELO PRODUTOR. META: %d/%d\n", quantidade_musicas, META); 
}