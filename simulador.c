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
	char numero_serie[36];
	char porto_destino[4];
	char *marca_tempo_entrada;
	char *marca_tempo_saida; 
};

struct Contentor queue[MAX];

void push(struct Contentor contentor);
struct Contentor pop();
void clean(int i);
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
	int nread;
	do {
		char buffer[41];
		struct Contentor contentor;
		while((nread = read(fifoFD, buffer, sizeof(buffer))) != 0) {
			int parte = 0;
			int index = 0;
			for (int i = 0; i < 41; i++) {
				if (parte == 1) {
					contentor.porto_destino[index] = buffer[i];
				}
				if (buffer[i] == ' ') {
					parte = 1;
					index = 0;
				}
				if (parte == 0) {
					contentor.numero_serie[index] = buffer[i];
				}
				index++;
			}
			push(contentor);
			//Adicionar na queue e esperar 1 seg
			//Depois tira da queue mete na matriz e fica entre 1 a 5 segundos lá
			//Depois tira da matriz e mete numa das queues para escrever no fifo e espera no 2 seg antes de escrever
			//Escrever no fifo
		}
	} while (fifoId == TREADERS - 1);
	display();
	close(fifoFD);
	pthread_exit(0);
}

void push(struct Contentor contentor) {
	int index;
	index = ((in++) & (MAX - 1));
	for (int i = 0; i < strlen(contentor.numero_serie); i++) {
		queue[index].numero_serie[i] = contentor.numero_serie[i];
	}
	for (int i = 0; i < strlen(contentor.porto_destino); i++) {
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
	for (int j = 0; i < strlen(queue[i].numero_serie); j++) {
		queue[i].numero_serie[j] = NULL;
	}
	for (int j = 0; i < strlen(queue[i].porto_destino); j++) {
		queue[i].porto_destino[j] = NULL;
	}
    queue[i].marca_tempo_entrada = NULL;
    queue[i].marca_tempo_saida = NULL;
}