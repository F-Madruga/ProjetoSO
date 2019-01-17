#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define TREADERS 4

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
	char buffer[41];
	int nread;
	do {
		while((nread = read(fifoFD, buffer, sizeof(buffer))) != 0) {
			printf("%s", buffer);
		}
	} while (fifoId == TREADERS - 1);
	close(fifoFD);
	pthread_exit(0);
}