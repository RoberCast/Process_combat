/*************************************************
* PROGRAM NAME: child.c
* AUTHOR:       Roberto Castillejo
* VERSION:      1.0
*************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

/* Declaration of the message that CHILD sends with the result to FATHER */
struct TipoMensaje{
  long pid; 
  char resultado[2];
};

struct TipoMensaje mensajeHijo;

/***********************************
*	PROTOTYPE OF FUNCTIONS
***********************************/

void defender();
void indefenso();
int atacarHijo(int lista[], int numHijos);
/* Functions used by the semaphore */
int init_sem(int semid, ushort valor);
int wait_sem(int semid);
int signal_sem(int semid);


char estado[3];   /* Save the child's status */

/**********************************
*	MAIN PROGRAM
**********************************/
void main(int argc, char *argv[])
{
  if(argc < 2)
  {
    printf("Error. Parameters are missing.\n");
    fflush(stdout);
    exit(1);
  }
  else{
    int descriptorBarrera = atoi(argv[1]);  /* Save the pipe reading descriptor */
    int numHijos = atoi(argv[2]);           /* The number of children */
    key_t llave = atoi(argv[3]);            /* Store a key */
    int *lista;                             /* Array of N pids, N iS numHijos */
    int shmid;                              /* Shared memory area */
    int mensajes;                           /* Save a message queue */
    int sem; 	                            /* Save the semaphore */
    int longitud = sizeof(mensajeHijo)-sizeof(mensajeHijo.pid);  /* Length of mensajeHijo */

    /* Resets the message queue with read and modify permissions for the user */
    if((mensajes=msgget(llave, 0600)) == -1)
    {
      perror("semid");
      exit(2);
    }

    /* Resets the shared memory area with read and write permissions */ 
    if((shmid=shmget(llave, sizeof(int)*numHijos, 0600)) == -1)
    {
      perror("shmget");
      exit(3);
    }
    lista = shmat(shmid, 0, 0);

    /* Resets the semaphore */ 
    if((sem=semget(llave, 1, 0600)) == -1)
    {
      perror("Semget failure");
      exit(1);
    }

    char mensaje[numHijos];

    /* Receives a byte from the pipe by the father */
    while(read(descriptorBarrera, mensaje, 1) > 0){
      /* Preparation phase */
      strcpy(estado,""); /* Initializes state to empty string */
      /* Randomly choose whether to attack or defend */
      srand(getpid()+time(0));
      int eleccion = rand() % 2 + 1;

      /* Action taken: Defend or attack */
      if(eleccion == 1)
      {
        signal(SIGUSR1,defender);
        usleep(200000); /* It sleeps for 0.2s, which is 200,000 microseconds */
      }
      else{    /* randomFunction is worth 2 */
        signal(SIGUSR1,indefenso);
        usleep(100000); /* It sleeps for 0.1s, which is 100,000 microseconds */
        int hijoObjetivo = -1;
        wait_sem(sem);
        hijoObjetivo = atacarHijo(lista, numHijos);
        signal_sem(sem);
        printf("%d is attacking the child %d\n", getpid(), hijoObjetivo);
        fflush(stdout);
        kill(hijoObjetivo,SIGUSR1);
        usleep(100000);
      }

      /* Sending status message */
      mensajeHijo.pid = getpid();
      strcpy(mensajeHijo.resultado, estado);
      if(msgsnd(mensajes, &mensajeHijo, longitud, 0) == -1)
      {
        perror("msgsnd");
        exit(2);
      }
    }
  }
}

/*****************************************
*	IMPLEMENTATION OF FUNCTIONS
*****************************************/

/*************************************************
*  Defender function. Changes the status to "OK" 
*  and prints a message to standard output.
*************************************************/
void defender()
{
  strcpy(estado,"OK");
  printf("The child %d has repelled an attack.\n", getpid());
  fflush(stdout);
}

/**************************************************
*  Helpless function. Changes the state to "KO" 
*  and prints a message to standard output.
**************************************************/
void indefenso()
{
  strcpy(estado,"KO");
  printf("The child %d has been ambushed while carrying out an attack.\n", getpid());
  fflush(stdout);
}

/******************************************************
*  Function that returns a target child of the attack
******************************************************/
int atacarHijo(int lista[], int numHijos)
{
  /* Randomly choose the target child */
  int pidObjetivo = 0;
  while(pidObjetivo == 0 || pidObjetivo == getpid()){ 
    int randomindex = rand() % numHijos;
    pidObjetivo = lista[randomindex]; 
  } 
  return pidObjetivo; 
}

/********************************************
*  Semaphore initialization function
********************************************/
int init_sem(int semid, ushort valor)
{
  ushort sem_array[1];
  sem_array[0] = valor;
  if(semctl(semid, 0, SETALL, sem_array) == -1)
  {
    perror("Error semctl");
    return -1;
  }
  return 0;
}

/******************************************
*  Check function, P(), of the semaphore
******************************************/
int wait_sem(int semid)
{
  struct sembuf op[1];
  op[0].sem_num = 0;
  op[0].sem_op = -1; /* Operation P, decrement one */
  op[0].sem_flg = 0;
  if(semop(semid, op, 1) == -1)
  {
    perror("Error semop:");
    return -1;
  }
  return 0;
}

/**********************************************
*  Increment function, V(), of the semaphore
**********************************************/
int signal_sem(int semid)
{
  struct sembuf op[1];
  op[0].sem_num = 0;
  op[0].sem_op = 1; /* Operation V, add one */
  op[0].sem_flg = 0;
  if(semop(semid, op, 1) == -1)
  {
    perror("Error semop:");
    return -1;
  }
  return 0;
}
