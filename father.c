/*************************************************
* PROGRAM NAME: father.c
* AUTHOR:       Roberto Castillejo
* VERSION:      1.0
*************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>

/* Declaration of the message received by FATHER with the result of CHILD */
struct TipoMensaje{
  long pid; 
  char resultado[2];
};

struct TipoMensaje mensajeHijo;

/**********************************
*	PROTOTYPE OF FUNCTIONS
**********************************/

/* Functions used by the semaphore */
int init_sem(int semid, ushort valor);
int wait_sem(int semid);
int signal_sem(int semid);

/***************************
*	MAIN PROGRAM
***************************/
void main(int argc, char *argv[])
{
  if(argc < 2)
  {
    printf("Error. Parameters are missing.\n");
    exit(1);
  }
  else{
    int numHijos = atoi(argv[1]); /* The number of children passed through the main argument */
    int k = numHijos;		  /* Number of alive children */
    int status;                   /* Status for waiting for the father */ 
    int pid;                      /* Stores the PID */
    int *lista;                   /* Array of N pids, N is numHijos */
    key_t llave;                  /* Store a key */
    int mensajes;                 /* Save a message queue */
    int sem; 	                  /* Save the semaphore */
    int shmid;                    /* Shared memory area */
    int barrera[2];               /* The Nameless Pipe */
    int longitud = sizeof(mensajeHijo)-sizeof(mensajeHijo.pid);  /* Length of mensajeHijo */    

    /* Create a key associated with the executable file and the letter X */
    if((llave=ftok("FATHER",'X')) == -1)
    {
      perror("ftok");
      exit(1);      
    }

    /* Create a message queue with read and modify permissions for the user */
    if((mensajes=msgget(llave, IPC_CREAT | 0600)) == -1)
    {
      perror("semid");
      exit(2);
    }

    /* Create a shared memory area with read and write permissions. Obtain a pointer to the shared memory and assign it to the array arrayHijos */
    if((shmid=shmget(llave, sizeof(int)*numHijos, IPC_CREAT | 0600)) == -1)
    {
      perror("shmget");
      exit(3);
    }
    lista = shmat(shmid, 0, 0);

    /* Create an unnamed pipe */
    if(pipe(barrera) == -1)
    {
      perror("pipe");
      exit(-1);
    } 

    /* Create a private semaphore and initialize it to 0 */ 
    if((sem=semget(llave, 1, IPC_CREAT | 0600)) == -1)
    {
      perror("Semget failure");
      exit(1);
    }
    init_sem(sem, 1);

    /* Arguments for the execution of child.c */
    char descriptor[sizeof(barrera[0])];   /* Save the pipe reading descriptor */
    char llaveHijo[sizeof(llave)];         /* Save the generated key to pass it on to the child */
    sprintf(descriptor, "%d", barrera[0]); /* Convert the descriptor to a string to pass it as a parameter */
    sprintf(llaveHijo, "%d", llave);       /* Convert the key to a string to pass it as a parameter */
    char *args[] = {"./CHILD", descriptor, argv[1], llaveHijo,  NULL};   /* Arguments for execvp() */

    /* N CHILD processes are created */
    for(int i=0; i<numHijos; i++)
    {
      if((pid=fork()) == -1)
      {
        perror("fork");
        exit(3);
      }
      if(pid == 0)  /* Child */
      {
        /* Run the child code */
        execvp(args[0],args);
      }
      else{ /* Father */
        wait_sem(sem);
        lista[i]=pid; /* Save the PID in the list (shared memory) */
        signal_sem(sem);
      }
    }
    
    /* Rounds of attacks */
    int ronda = 1; /* Guarda el numero de ronda actual */
    while(k>1){
      printf("\nStarting round of attacks %d.\n", ronda);
      printf("================================\n");
      fflush(stdout);
      /* Send K bytes to the pipe, one for each living child */
      char mensaje[k];
      write(barrera[1], mensaje, k);

      struct TipoMensaje mensajesK[k];  /* Save the messages from the children of the current round */
 

      /* Receive messages from your children */ 
      for(int i=0; i<k; i++){
        if(msgrcv(mensajes, &mensajeHijo, longitud, 0, 0) == -1)
        {
          perror("msgrcv");
          exit(3);
        }
        mensajesK[i] = mensajeHijo;
      }     

      /* Check the result of the contest */
      int k_mensajes = k;  /* Save the number of messages from your children */
      for(int i=0; i<k_mensajes; i++){
        if(strcmp(mensajesK[i].resultado, "KO") == 0)
        {
          for(int j=0; j<numHijos; j++){
            wait_sem(sem);
            if(mensajesK[i].pid == lista[j])
            {
              kill(lista[j], SIGTERM); /* Kill the child */
              wait(&status);	       /* Wait until the child dies */
              lista[j] = 0;            /* Sets the child's position in the list to 0  */
              k--;                     /* Update k */
            }
            signal_sem(sem);
          }
         }
      }
      ronda++;         /* The number of rounds is increased */
    }

    /* There is a winner or a tie */
    if(k < 2){
      int ganador = -1;  /* Save the winner's PID if there is one */
      FILE *fifo = fopen("result", "w"); /* Open the resulting FIFO file to write to it */
      if(k == 0)
      {
        fprintf(fifo, "\nTie.\n");
        fflush(stdout);
      }
      else if(k == 1)
      {
        for(int i=0; i<numHijos && ganador == -1; i++){
          wait_sem(sem);
          if(lista[i] != 0)
          {
            ganador = lista[i];
          }
          signal_sem(sem);
        }
        kill(ganador, SIGTERM);   /* It ends with the surviving child */
        wait(&status);            /* Wait until the surviving child finishes */
        fprintf(fifo, "\nThe child %d has won.\n", ganador);
        fflush(stdout);
      }
      fclose(fifo); /* Close the FIFO result */
    }

    printf("\n\n");
    fflush(stdout);

    /* Both ends of the pipe are closed */
    close(barrera[0]);
    close(barrera[1]);
    
    /* Eliminates IPCS */
    shmdt(lista);                  /* Free up the list array */
    shmctl(shmid, IPC_RMID, 0);    /* Free up shared memory */
    msgctl(mensajes, IPC_RMID, 0); /* The message queue is closed */
    semctl(sem,IPC_RMID, 0);       /* The semaphore is closed */
    
    /* Call system to demonstrate that IPC resources have been released by showing message queues and semaphores */
    system("ipcs -qs");
    exit(0);
  }
}

/****************************************
*	IMPLEMENTATION OF FUNCTIONS
****************************************/

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

/******************************************
*  Increment function, V(), of the semaphore
******************************************/
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
