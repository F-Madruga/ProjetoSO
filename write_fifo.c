#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "input_creator.h"

#define MAXTHREADS 4

void *createFifo(void* fifo);
void *writeFifo(void* fifo);

int main(int argc, char *argv[])
{
	if(argc != 2){
		printf("Usage: %s <numero de linhas> \n" ,argv[0]);
		exit(-1);
	}
	createInput(atoi(argv[1]));
	pthread_t threadsId[MAXTHREADS];
	//Criação das threads que criam os fifos
	int fifoId[MAXTHREADS];
	for (int i = 0; i < MAXTHREADS; i++) {
		fifoId[i] = i;
		pthread_create(&threadsId[i], NULL, createFifo, (void *)&fifoId[i]);
	}
	//Espera das threads que criam os fifos
	for (int i = 0; i < MAXTHREADS; i++) {
		pthread_join(threadsId[i], NULL);
	}
	return 0;
}

void *createFifo(void* fifo) {
	int fifoId = *(int *)fifo;
	char fifoName[15];
	sprintf(fifoName, "fifo%d", fifoId);

	//Apaga o fifo caso ele já exista
	if (access(fifoName, F_OK) != -1 ) {
		remove(fifoName);
	}

	//Cria o fifo
	if (mkfifo(fifoName, 0666) != 0) {
    	perror("mkfifo() error");
  	}
  	if (fifoId != MAXTHREADS - 1) {
		writeFifo(fifo);
	}
	pthread_exit(0);
}

void *writeFifo(void* fifo) {
	int numLinhas = 0;
	int fifoId = *(int *)fifo;
	char fifoName[15];
	sprintf(fifoName, "fifo%d", fifoId);
	int fifoFD;
	fifoFD = open(fifoName, O_WRONLY);

	char fileName[15];
	sprintf(fileName, "input%d.dat", fifoId);

	FILE* fp;
	char buffer[41];

	fp = fopen(fileName, "r");

	while(fgets(buffer, 41, (FILE*) fp)) {
    	write(fifoFD, buffer, strlen(buffer));
    	sleep(1);
	}
	fclose(fp);
	close(fifoFD);
	pthread_exit(0);
}