#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define TREADERS 4
#define MAX 16

int in = 0;
int out = 0;

struct Contentor
{
	char numero_serie[37];
	char porto_destino[3];
	char *marca_tempo_entrada;
	char *marca_tempo_saida; 
};

struct Contentor queue[MAX];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void push(struct Contentor contentor);
struct Contentor pop();
void clean(int i);
void display();
int isFull();

void *readFifo(void* fifo);
void *deque(void* arg);

int main() {
	pthread_t threadsId[TREADERS];
	pthread_t dequeThread;
	int fifoId[TREADERS];
	pthread_create(&dequeThread, NULL, deque, (void *)NULL);
	for (int i = 0; i < TREADERS; i++) {
		fifoId[i] = i;
		pthread_create(&threadsId[i], NULL, readFifo, (void *)&fifoId[i]);
	}
	pthread_join(dequeThread, NULL);
	for (int i = 0; i < TREADERS; i++) {
		pthread_join(threadsId[i], NULL);
	}		
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
			pthread_mutex_lock(&mutex);
			push(contentor);
			pthread_mutex_unlock(&mutex);
		}
	} while (fifoId == TREADERS - 1);
	close(fifoFD);
	pthread_exit(0);
}

void *deque(void* arg) {
	printf("ola\n");
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
    queue[i].marca_tempo_entrada = NULL;
    queue[i].marca_tempo_saida = NULL;
}

int isFull() {
	if (((in) & (MAX - 1)) == 0) {
		return 1;
	}
	return 0;
}