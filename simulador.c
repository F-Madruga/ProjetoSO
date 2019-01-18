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
	char *numero_serie;
	char *porto_destino;
	char *marca_tempo_entrada;
	char *marca_tempo_saida; 
};

struct Contentor queue[MAX];

void push(struct Contentor contentor);
struct Contentor pop();
void display();
void *readFifo(void* fifo);

int main() {
	pthread_t threadsId[TREADERS];
	int fifoId[TREADERS];
	for (int i = 0; i < TREADERS; i++) {
		fifoId[i] = i;
		pthread_create(&threadsId[i], NULL, readFifo, (void *)&fifoId[i]);
	}
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
	getchar();
	char buffer[41];
	int nread;
	do {
		struct Contentor contentor;
		while((nread = read(fifoFD, buffer, sizeof(buffer))) != 0) {
			char *token;
			token = strtok(buffer, " ");
			int dados = 0;
			while (token != NULL) {
				if (dados == 0) {
					contentor.numero_serie = token;
				}
				if (dados == 1) {
					contentor.porto_destino = token;
				}
				token = strtok(NULL, " ");
				dados++;
			}
			push(contentor);
			//Adicionar na queue e esperar 1 seg
			//Depois tira da queue mete na matriz e fica entre 1 a 5 segundos lá
			//Depois tira da matriz e mete numa das queues para escrever no fifo e espera no 2 seg antes de escrever
			//Escrever no fifo
		}
	} while (fifoId == TREADERS - 1);
	//display();
	close(fifoFD);
	pthread_exit(0);
}

void push(struct Contentor contentor) {
	int index;
	index = ((in++) & (MAX - 1));
	queue[index].numero_serie = contentor.numero_serie;
	queue[index].porto_destino = contentor.porto_destino;
}

struct Contentor pop() {
	int index;
	index = ((out++) & (MAX - 1));
	struct Contentor contentor;
	contentor = queue[index];
	return contentor;
}

void display() {
	for (int i = 0; i < MAX; i++) {
		if (queue[i].numero_serie != NULL) {
			printf("%s, %s", queue[i].numero_serie, queue[i].porto_destino);
		}
	}
}