#ifndef SERVER_H
#define SERVER_H

#include "sharedData.h"
#include <pthread.h>
#include <signal.h>

typedef struct Klient {
  pthread_t thread;
  int socket;
} Klient;

typedef struct thread_data {
  Hra *hra;
  pthread_mutex_t *hraMutex;
  Klient klienti[100];
  int serverSocket;
} thread_data;

typedef struct klientThreadData {
  int hracSocket;
  thread_data *data;
} KlientThreadData;

void nakresliMapu(Hra *hra);
void nakresliHada(Had *had, Hra *hra);
void nakresliJedlo(Hra *hra);
void hadZjedolJedlo(Hra *hra, Had *had);
_Bool kontrolujKolizieHada(Hra *hra, Had *had);
void posunHada(Had *had, Hra *hra);
void nastavHru(Hra *hra);
void aktualizaciaHry(Hra *hra, Had *had);
int spustiServer(); 
void *klientVstup(void *arg);
void *serverLoop(void *arg);

#endif