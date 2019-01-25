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
#define TWRITTERS 10
#define MAX 16
#define LUGARES 10


struct Contentor
{
	char numero_serie[37];
	char porto_destino[4];
	long marca_tempo_entrada;
	long marca_tempo_saida; 
};

struct Contentor queue[TWRITTERS + 1][MAX];
int in[TWRITTERS + 1];
int out[TWRITTERS + 1];

struct Contentor estacionamento[LUGARES][LUGARES];
int xIn = -1;
int yIn = -1;
int active = 0;

const char *portos[TWRITTERS] = {"ANR", "BUS", "DXB", "GUA", "HAM", "HKG", "LAX", "RTM", "SHA", "SIN"};

sem_t sem1[TWRITTERS + 1];
sem_t sem2[TWRITTERS + 1];
pthread_mutex_t mutexEstacionamento;
pthread_mutex_t mutex;
pthread_mutex_t mutexes[LUGARES][LUGARES];

void push(struct Contentor contentor, int queueId);
struct Contentor pop(int queueId);
void clean(int i, int queueId);
void display(int queueId);
int isFull(int queueId);
int isEmpty(int queueId);
int porto_compare(char input[], char check[]);

void *readFifo(void* fifo);
void *dequeue(void* arg);
void *estacionar(void* contentor);
void *writeFifo(void* fifo);

int main() {
	for (int i = 0;i < TWRITTERS + 1; i++) {
		in[i] = 0;
		out[i] = 0;
		sem_init(&sem1[i], 0, 0);
		sem_init(&sem2[i], 0, MAX);
	}
	time_t t;
	srand((unsigned)time(&t));
	pthread_t threadsId[TREADERS];
	pthread_t threadsWrittersId[TWRITTERS];
	pthread_t dequeueThread;
	int fifoId[TREADERS];
	int fifoWritters[TWRITTERS];
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutexEstacionamento, NULL);
	for (int x = 0; x < LUGARES; x++) {
		for (int y = 0; y < LUGARES; y++) {
			pthread_mutex_init(&mutexes[x][y], NULL);
		}
	}
	
	pthread_create(&dequeueThread, NULL, dequeue, (void *)NULL);
	for (int i = 0; i < TREADERS; i++) {
		fifoId[i] = i;
		pthread_create(&threadsId[i], NULL, readFifo, (void *)&fifoId[i]);
	}

	for (int i = 0; i < TWRITTERS; i++) {
		fifoWritters[i] = i;
		pthread_create(&threadsWrittersId[i], NULL, writeFifo, (void *)&fifoWritters[i]);
	}
	
	for (int i = 0; i < TREADERS; i++) {
		pthread_join(threadsId[i], NULL);
	}
	active = 1;
	pthread_join(dequeueThread, NULL);

	for (int i = 0; i < TWRITTERS; i++) {
		pthread_join(threadsWrittersId[i], NULL);
	}	
	return 0;
}

void *readFifo(void* fifo) {
	int fifoId = *(int *)fifo;
	char fifoName[15];
	sprintf(fifoName, "FIFO-IN%d", fifoId);
	int fifoFD;
	fifoFD = open(fifoName, O_RDONLY);
	int nread;
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
		sem_wait(&sem1[0]);
		pthread_mutex_lock(&mutex);
		push(contentor, 0);
		sleep(1);
		pthread_mutex_unlock(&mutex);
		sem_post(&sem2[0]);
	}
	close(fifoFD);
	pthread_exit(0);
}

void *dequeue(void* arg) {
	while (active == 0 || isEmpty(0) == 0) {
		sem_wait(&sem2[0]);
		struct Contentor contentor;
		contentor = pop(0);
		if (contentor.numero_serie[0] != '\0') {
			time_t timeNow = time(NULL);
			contentor.marca_tempo_entrada = timeNow;
			pthread_t threadsId;
			pthread_create(&threadsId, NULL, estacionar,(void *)&contentor);
		}
		sem_post(&sem1[0]);
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
	pthread_mutex_unlock(&mutexEstacionamento);
	sleep((rand() + 1) % 6);
	for (int i = 0; i < TWRITTERS; i++) {
		if (porto_compare(estacionamento[x][y].porto_destino, portos[i])) {
			sem_wait(&sem1[i + 1]);
			push(estacionamento[x][y], i + 1);
			sleep(2);
			sem_post(&sem2[i + 1]);
			break;
		}
	}
	estacionamento[x][y].numero_serie[0] = '\0';
	estacionamento[x][y].porto_destino[0] = '\0';
    estacionamento[x][y].marca_tempo_entrada = 0;
    estacionamento[x][y].marca_tempo_saida = 0;
	pthread_mutex_unlock(&mutexes[x][y]);
	pthread_exit(0);
}

void *writeFifo(void* fifo) {
	int fifoId = *(int *)fifo;
	char fifoName[15];
	sprintf(fifoName, "FIFO-OUT%d", fifoId);

	//Apaga o fifo caso ele já exista
	if (access(fifoName, F_OK) != -1 ) {
		remove(fifoName);
	}

	//Cria o fifo
	if (mkfifo(fifoName, 0666) != 0) {
    	perror("mkfifo() error");
  	}
	int fifoFD;
	fifoFD = open(fifoName, O_WRONLY);
	while (active == 0 || isEmpty(fifoId + 1) == 0) {
		sem_wait(&sem2[fifoId + 1]);
		struct Contentor contentor;
		char linha[100];
		contentor = pop(fifoId + 1);
		if (contentor.numero_serie[0] != '\0') {
			sprintf(linha, "%s %s %ld %ld\n",contentor.numero_serie, contentor.porto_destino, contentor.marca_tempo_entrada, contentor.marca_tempo_saida);
			write(fifoFD, linha, sizeof(linha));
		}
		sem_post(&sem1[fifoId + 1]);
	}
	close(fifoFD);
	pthread_exit(0);
}

void push(struct Contentor contentor, int queueId) {
	int index;
	index = ((in[queueId]++) & (MAX - 1));
	for (int i = 0; i < sizeof(contentor.numero_serie); i++) {
		queue[queueId][index].numero_serie[i] = contentor.numero_serie[i];
	}
	for (int i = 0; i < sizeof(contentor.porto_destino); i++) {
		queue[queueId][index].porto_destino[i] = contentor.porto_destino[i];
	}
	queue[queueId][index].marca_tempo_entrada = contentor.marca_tempo_entrada;
    queue[queueId][index].marca_tempo_saida = contentor.marca_tempo_saida;
}

struct Contentor pop(int queueId) {
	int index;
	index = ((out[queueId]++) & (MAX - 1));
	struct Contentor contentor;
	contentor = queue[queueId][index];
	clean(index, queueId);
	return contentor;
}

void display(int queueId) {
	for (int i = 0; i < MAX; i++) {
		printf("%s, %s\n", queue[queueId][i].numero_serie, queue[queueId][i].porto_destino);
	}
}

void clean(int i, int queueId) {
	queue[queueId][i].numero_serie[0] = '\0';
	queue[queueId][i].porto_destino[0] = '\0';
    queue[queueId][i].marca_tempo_entrada = 0;
    queue[queueId][i].marca_tempo_saida = 0;
}

int isFull(int queueId) {
	if (((in[queueId]) & (MAX - 1)) == 0) {
		return 1;
	}
	return 0;
}

int isEmpty(int queueId) {
	if (out[queueId] == in[queueId]) {
		return 1;
	}
	return 0;
}

int porto_compare(char input[], char check[]) {
	int result = 1;
	for (int i = 0; i < 3; i++) {
		if (input[i] != check[i]) {
			result = 0;
			break;
		}
	}
	return result;
}