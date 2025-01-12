#ifndef KLIENT_H
#define KLIENT_H

#include "sharedData.h"
#include <ncurses.h>

typedef struct thread_data {
  Hra *hra;
  pthread_mutex_t *hraMutex;
  int socket;
  int hracId;
} thread_data;


void vykresliHada(Hra* hra);
void vykresliMapu(Hra *hra);
void *socketThreadLoop(void *arg);
void *inputLoop(void *arg);
void menu();

#endif