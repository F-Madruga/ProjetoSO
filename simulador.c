#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <time.h>

#define TREADERS 4
#define MAX 16
#define LUGARES 10


struct Contentor
{
	char numero_serie[37];
	char porto_destino[4];
	long marca_tempo_entrada;
	long marca_tempo_saida; 
};

struct Contentor queue[MAX];
int in = 0;
int out = 0;
int k = 0;

struct Contentor estacionamento[LUGARES][LUGARES];
int xIn = -1;
int yIn = -1;

sem_t sem1;
sem_t sem2;
pthread_mutex_t mutexEstacionamento;
pthread_mutex_t mutex;
pthread_mutex_t mutexes[LUGARES][LUGARES];

void push(struct Contentor contentor);
struct Contentor pop();
void clean(int i);
void display();
int isFull();
int isEmpty();

void *readFifo(void* fifo);
void *dequeue(void* arg);
void *estacionar(void* contentor);

int main() {
	time_t t;
	srand((unsigned)time(&t));
	pthread_t threadsId[TREADERS];
	pthread_t dequeueThread;
	int fifoId[TREADERS];
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutexEstacionamento, NULL);
	for (int x = 0; x < LUGARES; x++) {
		for (int y = 0; y < LUGARES; y++) {
			pthread_mutex_init(&mutexes[x][y], NULL);
		}
	}
	
	sem_init(&sem1, 0, 0);
	sem_init(&sem2, 0, MAX);
	pthread_create(&dequeueThread, NULL, dequeue, (void *)NULL);
	for (int i = 0; i < TREADERS; i++) {
		fifoId[i] = i;
		pthread_create(&threadsId[i], NULL, readFifo, (void *)&fifoId[i]);
	}
	
	for (int i = 0; i < TREADERS; i++) {
		pthread_join(threadsId[i], NULL);
	}	
	pthread_join(dequeueThread, NULL);
	return 0;
}

void *readFifo(void* fifo) {
	int fifoId = *(int *)fifo;
	char fifoName[15];
	sprintf(fifoName, "fifo%d", fifoId);
	int fifoFD;
	fifoFD = open(fifoName, O_RDONLY);
	int nread;
	do {
		char buffer[41];
		struct Contentor contentor;
		while((nread = read(fifoFD, buffer, sizeof(buffer))!=0)) {
			int i = 0;
			char *divisao = strtok (buffer, " ");
			char *referencias [2];
			while(divisao != NULL){
				referencias[i++] = divisao;
				divisao = strtok (NULL, " ");
			}
			memcpy(contentor.numero_serie, referencias[0], 36);
			memcpy(contentor.porto_destino, referencias[1], 3);
			sem_wait(&sem1);
			pthread_mutex_lock(&mutex);
			push(contentor);
			sleep(1);
			pthread_mutex_unlock(&mutex);
			sem_post(&sem2);
		}
	} while (fifoId == TREADERS - 1);
	close(fifoFD);
	pthread_exit(0);
}

void *dequeue(void* arg) {
	while (1) {
		sem_wait(&sem2);
		struct Contentor contentor;
		contentor = pop();
		if (contentor.numero_serie[0] != '\0') {
			time_t timeNow = time(NULL);
			contentor.marca_tempo_entrada = timeNow;
			pthread_t threadsId;
			pthread_create(&threadsId, NULL, estacionar,(void *)&contentor);
			//pthread_join(threadsId, NULL);
		}
		sem_post(&sem1);
	}
	pthread_exit(0);
}

void *estacionar(void* contentor) {
	struct Contentor container = *(struct Contentor *)contentor;
	pthread_mutex_lock(&mutexEstacionamento);
	if (xIn == -1 && yIn == -1) {
		xIn = 0;
		yIn = 0;
	}
	else {
		if (xIn < LUGARES - 1) {
			xIn++;
		}
		else {
			xIn = 0;
			if (yIn < LUGARES - 1) {
				yIn++;
			}
			else {
				yIn = 0;
			}
		}
	}
	int x = xIn;
	int y = yIn;
	pthread_mutex_lock(&mutexes[x][y]);
	estacionamento[x][y] = container;
	k++;
	printf("%s, %s - (%d, %d) num = %d\n", estacionamento[x][y].numero_serie, estacionamento[x][y].porto_destino, x, y,k);
	pthread_mutex_unlock(&mutexEstacionamento);
	sleep((rand() + 1) % 6);
	estacionamento[x][y].numero_serie[0] = '\0';
	estacionamento[x][y].porto_destino[0] = '\0';
    estacionamento[x][y].marca_tempo_entrada = 0;
    estacionamento[x][y].marca_tempo_saida = 0;
	printf("%s, %s - (%d, %d)\n", estacionamento[x][y].numero_serie, estacionamento[x][y].porto_destino, x, y);
	pthread_mutex_unlock(&mutexes[x][y]);
	pthread_exit(0);
}

void push(struct Contentor contentor) {
	int index;
	index = ((in++) & (MAX - 1));
	for (int i = 0; i < sizeof(contentor.numero_serie); i++) {
		queue[index].numero_serie[i] = contentor.numero_serie[i];
	}
	for (int i = 0; i < sizeof(contentor.porto_destino); i++) {
		queue[index].porto_destino[i] = contentor.porto_destino[i];
	}
	queue[index].marca_tempo_entrada = contentor.marca_tempo_entrada;
    queue[index].marca_tempo_saida = contentor.marca_tempo_saida;
}

struct Contentor pop() {
	int index;
	index = ((out++) & (MAX - 1));
	struct Contentor contentor;
	contentor = queue[index];
	clean(index);
	return contentor;
}

void display() {
	for (int i = 0; i < MAX; i++) {
		printf("%s, %s\n", queue[i].numero_serie, queue[i].porto_destino);
	}
}

void clean(int i) {
	queue[i].numero_serie[0] = '\0';
	queue[i].porto_destino[0] = '\0';
    queue[i].marca_tempo_entrada = 0;
    queue[i].marca_tempo_saida = 0;
}

int isFull() {
	if (((in) & (MAX - 1)) == 0) {
		return 1;
	}
	return 0;
}

int isEmpty() {
	if (out == in) {
		return 1;
	}
	return 0;
}