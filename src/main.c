#include "sock-lib.h"
#include "sems.h"
#include "pthread.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <fcntl.h>
#include <time.h>

#define SHM_SIZE 921600 /*640*480*3=frame->imageSize*/
#define MAXDATASIZE_TCP 10/* mÁxima cantidad de bytes que puede recibir en una transacciÓn*/
#define MAXDATASIZE_UDP 36864/*36kB por paquete*/
#define CLIENT_PORT 32000
#define SEM_CANT 3
#define SEM_WRITER 0
#define SEM_READERS_EMPTY 1
#define SEM_CANT_READERS 2

static const char* homePath = "/home/valentin";
static const char* keyPath = "/home/valentin/key";

int cantReaders=0;

struct connectionParams
{
  int width;
  int height;
  int fps;
};

struct threadParamsClient
{
  int fd;
};

struct threadParamsServer
{
    struct sockaddr_in their_addr;
    struct connectionParams cp;
    char *shmPtr;
    int semid;
};

void *threadFunctionClient(struct threadParamsClient *tpc)
{
      char bufTCP[MAXDATASIZE_TCP];
      int numbytes;

       while(1)
       {
             if( (numbytes=recv(tpc->fd,bufTCP,sizeof(bufTCP),0)) == -1)
             {
               perror("Error de lectura en el socket");
             }
             //printf("%s\n",bufTCP);
             if( (numbytes=send(tpc->fd,"ACK",sizeof(bufTCP),0)) == -1)
             {
              perror("Error en send");
             }
       }
}


void *threadFunctionServer(struct threadParamsServer *tp)
{
      char i;
      struct timespec t;
      int sockudp;
      struct sockaddr_in their_addr=tp->their_addr;
      char buf[MAXDATASIZE_UDP+1];
      pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
      /*OBTENGO ms DE ACUERDO A LOS FPS*/
      float msDelay=(1000.0/(tp->cp.fps) ) ;
      t.tv_sec=0;
      t.tv_nsec=1000000L*((unsigned int) (msDelay));
      printf  ("Nueva conexión:  %s\n", inet_ntoa(their_addr.sin_addr));
      printf  ("Puerto Cliente:  %u\n", ntohs(their_addr.sin_port));
      their_addr.sin_port=htons(32000);
    	if ((sockudp = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    	{
    		perror("Error en creación de socket");
    		exit(1);
    	}
      while(1)
      {
            /*SE ENVÍA EL FRAME EN BLOQUES, DONDE LA PRIMERA POSICIÓN DEL BLOQUE CONTIENE EL ÍNDICE DEL BLOQUE DENTRO DEL FRAME PARA QUE EL CLIENTE PUEDA ORDENARLOS*/
            if((p (tp->semid,SEM_WRITER)) == -1)
            {
              perror("P SEM_WRITER");
            }
            if((v (tp->semid,SEM_WRITER))==-1)
            {
              perror("V SEM_WRITER");
            }
            if((p (tp->semid,SEM_CANT_READERS)) == -1)
            {
              perror("P SEM_CANT_READERS");
            }
            if(cantReaders==0)
              if((p (tp->semid,SEM_READERS_EMPTY)) == -1)
              {
                perror("P SEM_READERS_EMPTY");
              }
            cantReaders++;
            if((v (tp->semid,SEM_CANT_READERS))==-1)
            {
              perror("V SEM_CANT_READERS");
            }
            //printf ("Thread: %lu SECCIÓN CRÍTICA\n",(unsigned long) pthread_self());
            /*SECCIÓN CRÍTICA*/
			      for(i=0;i< (((tp->cp.width)*(tp->cp.height)*3)/MAXDATASIZE_UDP);i++){
				         buf[0]=i;
			           memcpy(&(buf[1]),&(tp->shmPtr[i*MAXDATASIZE_UDP]),MAXDATASIZE_UDP);
				         sendto(sockudp,buf,MAXDATASIZE_UDP+1,0,(struct sockaddr *)&their_addr,sizeof(struct sockaddr));
            }
            /*FIN SECCIÓN CRÍTICA*/
            if((p (tp->semid,SEM_CANT_READERS)) == -1)
            {
              perror("P SEM_CANT_READERS");
            }
            cantReaders--;
            if(cantReaders==0)
                if((v (tp->semid,SEM_READERS_EMPTY))==-1)
                {
                  perror("V SEM_READERS_EMPTY");
                }
            if((v (tp->semid,SEM_CANT_READERS))==-1)
            {
              perror("V SEM_CANT_READERS");
            }
            /*DELAY DE ACUERDO A LOS FPS*/
            nanosleep(&t,(struct timespec *) NULL);
      }
      pthread_exit(0);
}



void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main (int argc, char *argv[])
{

  /*VARIABLES DE TIMEOUT*/
  fd_set set;
  struct timeval timeout;
  /*VARIABLES SEMÁFOROS*/
   int semid;
  /*VARIABLES OPENCV*/
   IplImage *frame;
   CvCapture *capture;
   char c;
  /*VARIABLES DE SHARED MEMORY*/
  key_t key;
  int shmid;
  char *shmPtr;
  /*VARIABLES DE THREADS*/
  pthread_t thread;
  pthread_attr_t attr;
  struct threadParamsServer tp;
  struct threadParamsClient tpc;
  /*VARIABLES DE CONEXIÓN*/
  int sockfd;			/* File Descriptor del socket por el que el servidor "escucha" conexiones*/
  struct sockaddr_in my_addr;	/* contendrá la dirección IP y el número de puerto local */
  struct sockaddr_in their_addr;
  struct sockaddr_in servaddr;
  int sockdup;
  int sockudp;
  int size;
  struct connectionParams cp;
  int numbytes;/*Contendrá el número de bytes recibidos por read () */
  char bufTCP[MAXDATASIZE_TCP];  /* Buffer donde se reciben los datos de read ()*/
  char bufUDP[MAXDATASIZE_UDP+1];
  char i;//
  /*VARIABLES DE PROCESOS*/
  pid_t pid;
  /*VARIABLES PARA CONTROLAR PROCESOS HIJOS*/
  struct sigaction sa;

  if (argc < 2)
  {
    fprintf(stderr,"Usage: ./video server or ./video [host ip] [port] [width] [height] [fps]\n");
    exit(1);
  }

  /*SERVER*/
  if (argc == 2)
  {
       /*SIGCHLD HANDLER*/
	     sa.sa_handler = sigchld_handler;
	     sigemptyset(&sa.sa_mask);
	     sa.sa_flags = SA_RESTART|SA_NOCLDSTOP; //SÓLO SE ACTIVA SIGCHLD EN CASO DE QUE UN HIJO TERMINE

	      if (sigaction(SIGCHLD, &sa, NULL) == -1)
        {
		        perror("sigaction");
		          exit(1);
	           }

      /*SHARED MEMORY INIT*/
      if ((key = ftok(keyPath, 'A')) == -1)
      {
        perror("ftok");
        exit(1);
      }
      if ((shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT)) == -1)
      {
        perror("shmget");
        exit(1);
      }
      shmPtr = shmat(shmid, (void *)0, 0);
      if (shmPtr == (char *)(-1))
      {
        perror("shmat");
        exit(1);
      }
      /*CREATE AND INITIALIZE SEMAPHORES*/
      if( (semid=sem_allocate(key,SEM_CANT,IPC_CREAT | 0666)) == -1)
      {
          perror("semget");
          exit(1);
      }
      if(sem_init (semid,SEM_WRITER) == -1)
      {
        perror("semctl");
        exit(1);
      }

      if(sem_init (semid,SEM_READERS_EMPTY) == -1)
      {
        perror("semctl");
        exit(1);
      }
      if(sem_init (semid,SEM_CANT_READERS) == -1)
      {
        perror("semctl");
        exit(1);
      }

      if ((sockfd = Open_conection (&my_addr)) == -1)	/*Open_conection realiza las llamadas a socket(), bind() y listen() */
      {
        perror ("Falló la creación de la conexión");
        exit (1);
      }

      switch(pid = fork()) {

        case -1:
          perror("Falló fork");
          close(sockdup);
          break;

        /*HIJO*/
        case 0:
                if( (capture = cvCaptureFromCAM(0)) == NULL)
                {
                  perror("Error al abrir cámara");
                  exit(1);

                }
                cvNamedWindow("Server Video",0);
                frame = cvQueryFrame(capture);
                while(1)
                {
                    frame = cvQueryFrame(capture);
                    if(frame!=0)
                    {
                      if((p(semid,SEM_WRITER)) == -1)
                      {
                        perror("P SEM_WRITER");
                      }

                      if((p(semid,SEM_READERS_EMPTY)) == -1)
                      {
                        perror("P SEM_READERS_EMPTY");
                      }
                      /*SECCIÓN CRÍTICA*/
                      memcpy(shmPtr,frame->imageData,frame->imageSize);
                      ///printf ("Proceso manejador de la webcam: SECCIÓN CRÍTICA\n");
                      /*FIN SECCIÓN CRÍTICA*/
                      if((v(semid,SEM_READERS_EMPTY))==-1)
                      {
                        perror("V SEM_READERS_EMPTY");
                      }
                      if((v(semid,SEM_WRITER))==-1)
                      {
                        perror("V SEM_WRITER");
                      }

                      cvShowImage("Server Video",frame);
                    }
                    cvWaitKey(30);
                 }

                cvReleaseCapture( &capture );
                cvDestroyWindow("Video");

                /* detach from the segment: */
                if (shmdt(shmPtr) == -1)
                {
                  perror("shmdt");
                  exit(1);
                }


        /*PADRE*/
        default:
        while(1)
        {
                  sockdup = Aceptar_pedidos (sockfd,&their_addr); /*Aceptar_pedidos devuelve el socket mediante el cual se comunicará con el cliente*/
                  switch(pid = fork())
                  {

                            case -1:
                                  perror("Falló fork");
                                  close(sockdup);
                                  break;

                          /*HIJO: Realizar comunicación con el nuevo socket: sockdup*/
                          case 0:
                                close(sockfd);		  //cerrar socket del servidor

                                /*CONECTARSE A LA SHARED MEMORY*/
                                if ((key = ftok(keyPath, 'A')) == -1)
                                {
                                  perror("ftok");
                                    exit(1);
                                }
                                if ((shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT)) == -1)
                                {
                                    perror("shmget");
                                    exit(1);
                                }
                                shmPtr = shmat(shmid, (void *)0, 0);
                                if (shmPtr == (char *)(-1))
                                {
                                  perror("shmat");
                                    exit(1);
                                }
                                if( (numbytes=  recv(sockdup,bufTCP,sizeof(bufTCP),0) ) ==-1)
                                {
                                  perror("Error de lectura en el socket");
                              		exit(1);
                              	}
                                cp.width=atoi(bufTCP);
                                if( (numbytes=  recv(sockdup,bufTCP,sizeof(bufTCP),0)) ==-1)
                                {
                                  perror("Error de lectura en el socket");
                              		exit(1);
                              	}
                                cp.height=atoi(bufTCP);
                                if( (numbytes=  recv(sockdup,bufTCP,sizeof(bufTCP),0)) ==-1)
                                {
                                  perror("Error de lectura en el socket");
                                  exit(1);
                                }
                                cp.fps=atoi(bufTCP);

                                /*SE RECIBIERON LOS PARÁMETROS DE CONEXIÓN CORRECTAMENTE=>INICIAR THREAD*/
                                tp.shmPtr=shmPtr;
                                tp.their_addr=their_addr;
                                tp.cp=cp;
                                tp.semid=semid;
                                pthread_attr_init(&attr);
                                pthread_create (&thread, &attr, (void*)threadFunctionServer, (void*)&tp);
                                /*INICIALIZACIÓN DEL SELECT*/
                                FD_ZERO(&set); /* clear the set */
                                FD_SET(sockdup, &set); /* add our file descriptor to the set */
                                timeout.tv_sec = 1;
                                timeout.tv_usec = 0;
                                while(1)
                                {
                                      if( (numbytes=send(sockdup,"ACK",sizeof(bufTCP),0)) == -1)
                                      {
                                        perror("Error send");
                                        exit(1);
                                      }
                                      select(sockdup+1, &set, NULL, NULL, &timeout);
                                      if (FD_ISSET(sockdup, &set))
                                      {
                                            if( (numbytes=recv(sockdup,bufTCP,sizeof(bufTCP),0)) == -1)
                                            {
                                              perror("Error de lectura en el socket");
                                            }
                                            //printf("CantBytes: %d\n",numbytes);
                                            //printf("%s\n",bufTCP);
                                            if(numbytes==0)
                                            {
                                             printf("Conexión finalizada con %s\n",inet_ntoa(their_addr.sin_addr) );
                                             pthread_cancel(thread);
                                             pthread_join(thread,NULL);
                                             close(sockdup);
                                             exit(1);
                                            }
                                            sleep(10);
                                      }
    	                                else
                                            break;

                                }
                                 /*END WHILE(1)*/



                          /*PADRE*/
                          default:
                                close(sockdup);
                                break;
                    break;
                    }
                    /*END FORK*/
            }
           break;
        }
        /*END FORK*/

    }
    /*END SERVER*/


    /*CLIENT*/
    if(argc == 6)
    {

            size=sizeof(struct sockaddr);
          	sockfd = conectar (argc, argv);
            send(sockfd,argv[3],sizeof(bufTCP),0);//WIDTH
            send(sockfd,argv[4],sizeof(bufTCP),0);//HEIGHT
            send(sockfd,argv[5],sizeof(bufTCP),0);//FPS

            /*INICIALIZACIÓN DEL THREAD PARA CONEXIÓN TCP*/
            tpc.fd=sockfd;
            pthread_attr_init(&attr);
            pthread_create (&thread, &attr, (void*)threadFunctionClient, (void*)&tpc);

            /*INICIALIZACIÓN PARA CONEXIÓN UDP*/
            if ((sockudp = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
            {
              perror("Error en creación de socket");
              exit(1);
            }
            bzero(&servaddr,sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr=inet_addr(argv[1]);
            servaddr.sin_port=htons(32000);
            bind(sockudp,(struct sockaddr *)&servaddr,sizeof(servaddr));

            /*CREACIÓN DE VENTANA Y FRAME DONDE SE ALMACENARA LA INFORMACIÓN RECIBIDA POR UDP*/
            cvNamedWindow("Client Video",0);
            frame=cvCreateImage(cvSize(atoi(argv[3]),atoi(argv[4]) ), IPL_DEPTH_8U, 3);
            while(1)
            {
              /*ORDENAR PAQUETES UDP*/
				      for(i=0;i<(((atoi(argv[3]))*(atoi(argv[4]))*3)/MAXDATASIZE_UDP);i++)
              {
					      numbytes=recvfrom(sockudp,bufUDP,MAXDATASIZE_UDP+1,0,(struct sockaddr *)&their_addr,&size);
                //printf("NumBytes: %d\n",numbytes);
                if(numbytes==MAXDATASIZE_UDP+1)//SI EL PAQUETE LLEGÓ ENTERO
					         memcpy(&(frame->imageData[bufUDP[0]*MAXDATASIZE_UDP]),&(bufUDP[1]),MAXDATASIZE_UDP); //LA POSICIÓN 0 DEL BUFFER CONTIENE EL ÍNDICE DEL PAQUETE DENTRO DEL FRAME
              }
                /*SE MUESTRA LA IMAGEN RECIBIDA*/
                cvShowImage("Client Video",frame);
                cvWaitKey(5);
              }
             cvDestroyWindow("Client Video");
             close(sockfd);
          	 return 0;
    }
    /*END CLIENT*/

return 0;

}
/*END MAIN*/


/*gcc video.c -o video `pkg-config --libs --cflags opencv` -ldl -lm -g -lsock -L. -lpthread*/

/*export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib*/
