#include <arpa/inet.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

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

typedef struct thread_data {
  Hra *hra;
  pthread_mutex_t *hraMutex;
  int socket;
  int hracId;
} thread_data;

void vykresliHada(Hra* hra) {
	for (int i = 0; i < hra->pocetHracov; ++i) {
		hra->mapa.mapa[hra->hraci[i].telo[0].y][hra->hraci[i].telo[0].x] = '@';
	}
}

void vykresliMapu(Hra *hra) {
  hra->mapa.sirkaMapa = MAPA_SIRKA;
  hra->mapa.dlzkaMapa = MAPA_DLZKA;

  for (int i = 0; i < MAPA_DLZKA; ++i) {
    for (int j = 0; j < MAPA_SIRKA; ++j) {
      mvprintw(i, j, "%c", hra->mapa.mapa[i][j]);
    }
  }
  vykresliHada(hra);
  refresh();
}

/*
void vypisHru(Hra *hra) {
    // Výpis mapy
    printf("Mapa:\n");
    for (int i = 0; i < MAPA_DLZKA; ++i) {
        for (int j = 0; j < MAPA_SIRKA; ++j) {
            printf("%c", hra->mapa.mapa[i][j]);
        }
        printf("\n");
    }
    // Výpis hráčov
    printf("\nHráči:\n");
    for (int i = 0; i < hra->pocetHracov; ++i) {
        printf("Hráč %d: dĺžka = %d, skóre = %d, zije = %d\n",
               i, hra->hraci[i].dlzka, hra->hraci[i].skore, hra->hraci[i].zije);
        printf("Pozície tela:\n");
        for (int j = 0; j < hra->hraci[i].dlzka; ++j) {
            printf("(%d, %d) ", hra->hraci[i].telo[j].x, hra->hraci[i].telo[j].y);
        }
        printf("\n");
    }

    // Počet hráčov a stav hry
    printf("\nPočet hráčov: %d\n", hra->pocetHracov);
    printf("Stav hry: %d\n", hra->bezi);
}
*/ 

void *socketThreadLoop(void *arg) {
  thread_data *data = (thread_data *)arg;

  while (data->hra->bezi) {
    Hra hraServer;
   // printf("Som v data hra bezi.\n");
    int len = recv(data->socket, &hraServer, sizeof(Hra), 0);
	if (len <= 0) {
     // data->hra->bezi = 0;
      break;
    }
	pthread_mutex_lock(data->hraMutex);
    *data->hra = hraServer;
	 vykresliMapu(data->hra);
    pthread_mutex_unlock(data->hraMutex);
  }
  return NULL;
}

int pripojSa(char *adresa, int port) {
  int klientSocket = socket(AF_INET, SOCK_STREAM, 0);
  while(klientSocket < 0) {
	  sleep(1);
  }
  struct sockaddr_in addr =
      (struct sockaddr_in){AF_INET, htons(port), inet_addr(adresa)};

  if (connect(klientSocket, (struct sockaddr *)&addr, sizeof(addr))) {
    fprintf(stderr, "err\n");
    return -1;
  }
  //printf("Napojilo ma v poriadku\n");
  return klientSocket;
}

void *inputLoop(void *arg) {
  thread_data *data = (thread_data *)arg;
  
  while (data->hra->bezi) {
    char in;
    scanf("%c", &in);
    pthread_mutex_lock(data->hraMutex);
    switch (in) {
    case 'q':
      data->hra->bezi = 0;
      break;
    default:
      break;
    }
	if (in != 'q') {
	char buf[2] = {(char)data->hracId, in};
    send(data->socket, &buf, 2, 0);
	//printf("%c" "%c", buf[0], buf[1]);
	}
    pthread_mutex_unlock(data->hraMutex);
  }
  // char in;
  return NULL;
}

void menu() {
	  clear();
	  mvprintw(0, 0, "========SLITHER========");
	  mvprintw(1, 0, " 1. Vytvor hru");
	  mvprintw(2, 0, " 2. Pripoj sa do hry");
	  mvprintw(3, 0, " 3. Odist");
	  mvprintw(4, 0, "=======================");
	  refresh();
}

int main(int argc, char **argv) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);	
	
  int vyber = 0;
  while(1) {
	  menu();
	  
	  vyber = getch();
	 if (vyber == '1') {
		 int childId = fork();
		 if (childId == 0) {
			 execl("./server", "slitherio_server", (char*) NULL);
			 perror("execl chyba");
			 mvprintw(0, 0, "Vytvaram server...");
			 return 0;
		 } else if (childId > 0) {
			 clear();
			 mvprintw(1, 0, "Pripajam klienta..");
			 refresh();
			 sleep(2);
			 break;
		 } else {
			 endwin();
			 return 1;
		 }
	 }
	 else if (vyber == '2') {
		 break;
	 } else if (vyber == '3') {
		 endwin();
		 return 0;
	 } else {
		 continue;
	 }
  }

 int klient = -1;

  while(klient < 0) {
	  klient = pripojSa("127.0.0.1", PORT);
	  if(klient < 0) {
		  sleep(1);
	  }
  }
  Hra hra;
  hra.bezi = 1;

 
  //printf("ncurses inicializovane\n");
  int hracId;
  if (recv(klient, &hracId, sizeof(hracId), 0) <= 0) {
    //hra.bezi = 0;
	//break;
  }
  //printf("Mam hracovo ID", hracId);
  pthread_mutex_t hraMutex;
  pthread_mutex_init(&hraMutex, NULL);
  thread_data threadData;
  threadData.hra = &hra;
  threadData.socket = klient;
  threadData.hraMutex = &hraMutex;
  threadData.hracId = hracId;

  pthread_t threadSocket;
 // printf("threadSocket created.\n");
  pthread_create(&threadSocket, NULL, socketThreadLoop, &threadData);
 // printf("threadSocket po vytvoreni\n");
  pthread_t threadInput;
 // printf("hrac input\n");
  pthread_create(&threadInput, NULL, inputLoop, &threadData);
 // printf("input created \n");


  while (hra.bezi) {
//	printf("Som vo while main hra bezi\n");
    sleep(1);
    pthread_mutex_lock(&hraMutex);
   //printf("%d\n", hra.pocetHracov);
   //printf("sirka: %d, dlzka: %d\n", hra.mapa.sirkaMapa, hra.mapa.dlzkaMapa);
    /*
    for (int i = 0; i < hra.mapa.dlzkaMapa; i++) {
      for (int j = 0; j < hra.mapa.sirkaMapa; j++) {
        printf("%c", hra.mapa.mapa[i][j]);
      }
      printf("\n");
    }
    printf("generoooo mapoo");
	*/
	vykresliMapu(&hra);
	mvprintw(0, MAPA_SIRKA, "Tvoje skore: %d", hra.hraci[hracId].skore);
    
    for (int i = 0; i < hra.pocetHracov; ++i) {
        mvprintw(0 + i + 1, MAPA_SIRKA + 3, "Hrac %d: %d", i, hra.hraci[i].skore);
    }
    
    pthread_mutex_unlock(&hraMutex);
	for (int i = 0; i < hra.pocetHracov; ++i) {
		if(!hra.hraci[i].zije) {
			mvprintw(MAPA_DLZKA, 0, "Umrel si.\n");
		}
	}

	
    pthread_mutex_unlock(&hraMutex);
    usleep(200000);
  }
  
  pthread_cancel(threadSocket);
  pthread_cancel(threadInput);
  pthread_join(threadSocket, NULL);
  pthread_join(threadInput, NULL);
  pthread_mutex_destroy(&hraMutex);

  close(klient);
  endwin();
}
