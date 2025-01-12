#include <arpa/inet.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

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

void nakresliMapu(Hra *hra) {
  for (int i = 0; i < MAPA_DLZKA; ++i) {
    for (int j = 0; j < MAPA_SIRKA; ++j) {
      if (i == 0 || i == MAPA_DLZKA - 1 || j == 0 || j == MAPA_SIRKA - 1) {
        hra->mapa.mapa[i][j] = '#';
      } else {
        hra->mapa.mapa[i][j] = ' ';
      }
    }
  }
//  printf("kresliiiiim\n");
}

void nakresliHada(Had *had, Hra *hra) {
  hra->mapa.mapa[had->telo[0].y][had->telo[0].x] = '@';
  for (int i = had->dlzka; i > 0; --i) {
    hra->mapa.mapa[had->telo[i].y][had->telo[i].x] = '=';
  }
}

void inicializujHraca(Hra *hra, int id) {
  int randomY, randomX;
  hra->hraci[id].dlzka = 0;
  hra->hraci[id].skore = 0;
  hra->hraci[id].zije = 1;
  do {
    randomY = rand() % ((MAPA_DLZKA - 2) - 1 + 1) + 1;
    randomX = rand() % ((MAPA_SIRKA - 2) - 1 + 1) + 1;
  } while (hra->mapa.mapa[randomY][randomX] != ' ');
  hra->hraci[id].telo[0].y = randomY;
  hra->hraci[id].telo[0].x = randomX;
  hra->mapa.mapa[randomY][randomX] = '@';
  hra->hraci[id].smer = 'd';
//  printf("had had \n");
}

void nakresliJedlo(Hra *hra) {
  int randomY, randomX;
  do {
    randomY = rand() % ((MAPA_DLZKA - 2) - 1 + 1) + 1;
    randomX = rand() % ((MAPA_SIRKA - 2) - 1 + 1) + 1;
  } while (hra->mapa.mapa[randomY][randomX] != ' ');
  hra->jedlo.y = randomY;
  hra->jedlo.x = randomX;
  hra->jedlo.jeZjedene = 0;
  hra->mapa.mapa[hra->jedlo.y][hra->jedlo.x] = '+';
}

void hadZjedolJedlo(Hra *hra, Had *had) {
  int hadHlavaY = had->telo[0].y;
  int hadHlavaX = had->telo[0].x;

  if(hra->mapa.mapa[hadHlavaY][hadHlavaX] == '+') {
	hra->mapa.mapa[hadHlavaY][hadHlavaX] = ' ';
	if (had->dlzka <= HAD_MAX_DLZKA) {
	  ++had->dlzka;
    }
    ++had->skore;
  }

  if (hra->jedlo.y == hadHlavaY && hra->jedlo.x == hadHlavaX) {
    hra->jedlo.jeZjedene = 1;
    hra->mapa.mapa[hra->jedlo.y][hra->jedlo.x] = ' ';
    if (had->dlzka <= HAD_MAX_DLZKA) {
      ++had->dlzka;
    }
    ++had->skore;
    nakresliJedlo(hra);
  }
}

_Bool kontrolujKolizieHada(Hra *hra, Had *had) {
  int hadHlavaY = had->telo[0].y;
  int hadHlavaX = had->telo[0].x;

  if (hra->mapa.mapa[hadHlavaY][hadHlavaX] == '#') {
	had->zije = 0;  
	for(int i = 0; i < had->dlzka; ++i) {
		hra->mapa.mapa[had->telo[i].y][had->telo[i].x] = '+';
		hra->mapa.mapa[hadHlavaY][hadHlavaX] = '+';
	}
	hra->mapa.mapa[hadHlavaY][hadHlavaX] = '#';
	return true;
  }
  
  for(int i = 1; i < had->dlzka; ++i) {
	if (had->telo[i].y == hadHlavaY && had->telo[i].x == hadHlavaX) {
		had->zije = 0;	  
		
		for(int j = 0; j < had->dlzka; ++j) {
			hra->mapa.mapa[had->telo[j].y][had->telo[j].x] = '+';
		}
	return true;
	}
  }
  
  for(int i = 0; i < hra->pocetHracov; ++i) {
	  Had *druhy = &hra->hraci[i];
	  if(druhy == had) continue;
	  for (int j = 0; j < druhy->dlzka; ++j) {
		  if(had->telo[j].y == hadHlavaY && had->telo[j].x == hadHlavaX) {
			  had->zije = 0;
			  for(int k = 0; k < had->dlzka; ++k) {
				  hra->mapa.mapa[had->telo[k].y][had->telo[k].x] = '+';
			  }
			  return true;
		  }
	  }
  }
  return false;
}

void posunHada(Had *had, Hra *hra) {
  int x = 0;
  int y = 0;
  char smer = had->smer;
  switch (smer) {
  case 'w':
    --y;
    break;
  case 's':
    ++y;
    break;
  case 'a':
    --x;
    break;
  case 'd':
    ++x;
    break;
  default:
    return;
  }
  /*
  if(had->zije) {
      int novaX = had->telo[0].x + x;
      int novaY = had->telo[0].y + y;
      
      // Skontrolovať, či nový smer je v rámci mapy
      if(novaX < 0 || novaX >= MAPA_SIRKA || novaY < 0 || novaY >= MAPA_DLZKA) {
          // Možno nastaviť had->zije = 0 alebo zmeniť smer, ak by ste chceli odraziť hada
          return;
      
	  }
  }  
  */
   if(kontrolujKolizieHada(hra, had)) {
	   return;
   }
  
   if(had->zije) {		
	for (int i = had->dlzka; i > 0; --i) {
		had->telo[i] = had->telo[i - 1];
	}
	had->telo[0].y += y;
	had->telo[0].x += x;
  }
}

void nastavHru(Hra *hra) {
 // printf("Nastavil som mapu.\n");
  nakresliMapu(hra);
 // printf("Nakreslil som mapu.\n");
  //nakresliJedlo(hra);
 // printf("Nakreslil som jedlo.\n");
  hra->bezi = 1;
//  printf("Hra bezi \n");
  hra->pocetHracov = 0;
}


void aktualizaciaHry(Hra *hra, Had *had) {
// printf("som v akt hr\n"); 
 nakresliMapu(hra);
//  printf("kreslim mapu\n");
  for (int i = 0; i < hra->pocetHracov; ++i) {
	if(had->zije && !kontrolujKolizieHada(hra, &hra->hraci[i])) {
      posunHada(&hra->hraci[i], hra);
      kontrolujKolizieHada(hra, &hra->hraci[i]);
      hadZjedolJedlo(hra, &hra->hraci[i]);
    }
	
  }
  
    for (int i = 0; i < hra->pocetHracov; ++i) {
      if (hra->hraci[i].zije) {
        nakresliHada(&hra->hraci[i], hra); 
      }
    }
    // Jedlo sa tiež treba znova zakresliť, ak nebolo zjednuté
    if (!hra->jedlo.jeZjedene) {
      hra->mapa.mapa[hra->jedlo.y][hra->jedlo.x] = '+';
    }
	
 // printf("idem spat\n");
  usleep(200000);
}

int spustiServer() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr =
      (struct sockaddr_in){AF_INET, htons(PORT), INADDR_ANY};

  int opt = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
    fprintf(stderr, "err bind socket\n");
    return 2;
  }
  listen(sock, 5);
  return sock;
}

typedef struct klientThreadData {
  int hracSocket;
  thread_data *data;
} KlientThreadData;

void *klientVstup(void *arg) {
  KlientThreadData *data = arg;
  int hracSocket = data->hracSocket;
  while (data->data->hra->bezi) {
    char vstup[2];
    if (recv(hracSocket, vstup, 2, 0) <= 0) {
      pthread_mutex_lock(data->data->hraMutex);
    //  printf("hrac sa odpojil.\n");
      data->data->hra->pocetHracov--;
      pthread_mutex_unlock(data->data->hraMutex);
      break;
    }
	
   // printf("klient vstup vstuuup %d \n", recv);
    pthread_mutex_lock(data->data->hraMutex);
   // printf("vstup od klienta %d: %c\n", vstup[0], vstup[1]);
	
	int hracoveId = vstup[0];
    switch (vstup[1]) {
    case 'l':
      // iba pre odskusanie
      data->data->hra->mapa.dlzkaMapa++;
      break;
    case 'k':
      data->data->hra->mapa.dlzkaMapa--;
      break;
    case 'w':
	  data->data->hra->hraci[hracoveId].smer = 'w';
      break;
    case 's':
	  data->data->hra->hraci[hracoveId].smer = 's';
      break;
    case 'a':
	  data->data->hra->hraci[hracoveId].smer = 'a';
      break;
    case 'd':
	  data->data->hra->hraci[hracoveId].smer = 'd';
      break;
    default:
      break;
    }
    pthread_mutex_unlock(data->data->hraMutex);
  }
  return NULL;
}

void *serverLoop(void *arg) {
  thread_data *data = arg;
  KlientThreadData klientData;
  while (data->hra->bezi) {
    int novyKlient = accept(data->serverSocket, NULL, NULL);
    if(novyKlient <= 0) {
   //   printf("Neprijal som klienta.\n");
      break;
    }
  //  printf("Prijal som klienta\n");
    pthread_mutex_lock(data->hraMutex);
    data->klienti[data->hra->pocetHracov].socket = novyKlient;

    inicializujHraca(data->hra, data->hra->pocetHracov);
 //   printf("Inicializujem noveho hraca.\n");
	
    int id = data->hra->pocetHracov++;
	nakresliJedlo(data->hra);
    if(send(novyKlient, &id, sizeof(id), 0) <= 0) {
  //    printf("Zle id.\n");
      break;
    }
  //  printf("dobre id\n");
    if(send(novyKlient, data->hra, sizeof(Hra), 0) <= 0) {
  //    printf("Chyba hra.\n");
      break;
    }
//   printf("vsetko v pohode\n");
    pthread_mutex_unlock(data->hraMutex);
    pthread_t klientThread;

    klientData.data = data;
    klientData.hracSocket = novyKlient;
//	printf("thread klient\n");
    pthread_create(&klientThread, NULL, klientVstup, &klientData);
//	printf("thread klient done\n");
  }
  return NULL;
}

int main(int argc, char **argv) {
  signal(SIGINT, SIG_IGN); 
  int serverSocket = spustiServer();
  srand(time(NULL));
  Hra hra;
  nastavHru(&hra);

  pthread_mutex_t hraMutex;
  pthread_mutex_init(&hraMutex, NULL);

  thread_data threadData;
  threadData.hra = &hra;
  threadData.hraMutex = &hraMutex;
  threadData.serverSocket = serverSocket;

  pthread_t threadSocket;
 // printf("threadsocket serverloop\n");
  pthread_create(&threadSocket, NULL, serverLoop, &threadData);
 // printf("thread socket created\n");
  while (hra.bezi) {
    pthread_mutex_lock(&hraMutex);
 //   printf("hra bezi\n");
 //   printf("%d\n", hra.pocetHracov);
    
    for (int i = 0; i < hra.pocetHracov; i++) {
      send(threadData.klienti[i].socket, &hra, sizeof(Hra), 0);
  //    printf("posielam thread\n");
    }
//	printf("po threade threade \n");
	for(int i = 0; i < hra.pocetHracov; ++i) {
		aktualizaciaHry(&hra, &hra.hraci[i]);
	}
//	printf("Po aktualizacii hry v maine\n");
    pthread_mutex_unlock(&hraMutex);
    usleep(200000);
  }
//  printf("hra skoncila\n");
 
  for (int i = 0; i < threadData.hra->pocetHracov; i++) {
    pthread_cancel(threadData.klienti[i].thread);
    pthread_join(threadData.klienti[i].thread, NULL);
  }

  pthread_mutex_destroy(&hraMutex);
  close(serverSocket);
}
