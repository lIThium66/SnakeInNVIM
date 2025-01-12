#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <arpa/inet.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define MAPA_SIRKA 50
#define MAPA_DLZKA 30
#define MAX_HRACI 50
#define HAD_MAX_DLZKA 100
#define PORT 6282

typedef struct Telo {
    int x, y;
} Telo;

typedef struct Had {
    Telo telo[HAD_MAX_DLZKA];
    int dlzka;
    int skore;
    _Bool zije;
    char smer;
} Had;

typedef struct Mapa {
    char mapa[MAPA_DLZKA][MAPA_SIRKA];
    int sirkaMapa;
    int dlzkaMapa;
} Mapa;

typedef struct Jedlo {
    int x, y;
    _Bool jeZjedene;
} Jedlo;

typedef struct Hra {
    Mapa mapa;
    Jedlo jedlo;
    Had hraci[MAX_HRACI];
    int pocetHracov;
    atomic_bool bezi;
} Hra;

#endif 