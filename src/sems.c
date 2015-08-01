#include "sems.h"


int sem_allocate (key_t key,int cant, int sem_flags)
{
  return semget (key, cant, sem_flags);
}

int sem_remove (int semid)
{
  union semun ignored_argument;
  return semctl (semid, 1, IPC_RMID, ignored_argument);
}

int sem_init (int semid, int semnum)
{
   union semun arg;
   arg.val = 1;
   return semctl(semid, semnum, SETVAL, arg);
 }

/*P*/
int p (int semid,int semnum)
{
  struct sembuf sb = {semnum, -1, SEM_UNDO};
  return semop (semid, &sb, 1);
}
/*V*/
int v (int semid,int semnum)
{
  struct sembuf sb = {semnum, 1, SEM_UNDO};
  return semop (semid, &sb, 1);
}
