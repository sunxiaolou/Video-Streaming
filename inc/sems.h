#ifndef SEMS_H
#define SEMS_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short int *array;
  struct seminfo *__buf;
};

int sem_allocate (key_t key,int cant, int sem_flags);
int sem_remove (int semid);
int sem_init (int semid, int semnum);
int p (int semid,int semnum);
int v (int semid,int semnum);
#endif
