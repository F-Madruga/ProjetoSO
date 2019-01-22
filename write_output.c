#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#define MAXTHREADS 10

struct Contentor
{
	char numero_serie[37];
	char porto_destino[4];
	long marca_tempo_entrada;
	long marca_tempo_saida; 
};
int fileFD;
pthread_mutex_t mutex360;

void *writeOutput(void * fifo);

int main()
{
    if (access("OUTPUT.txt", F_OK) != -1 ) {
		remove("OUTPUT.txt");
	}
    fileFD = open("OUTPUT.txt", O_WRONLY | O_CREAT, 0666);
    pthread_t threadsId[MAXTHREADS];
    int fifoId[MAXTHREADS];
    pthread_mutex_init(&mutex360, NULL);
    for (int i = 0; i < MAXTHREADS; i++) {
		fifoId[i] = i;
		pthread_create(&threadsId[i], NULL, writeOutput, (void *)&fifoId[i]);
	}
	
	for (int i = 0; i < MAXTHREADS; i++) {
	    pthread_join(threadsId[i], NULL);
    }
    return 0;
}

void *writeOutput(void *fifo) {
    int fifoId = *(int *)fifo;
    char fifoName[15];
	sprintf(fifoName, "FIFO-OUT%d", fifoId);
    int fifoFD;
    fifoFD = open(fifoName, O_RDONLY);
    int nread;
    char buffer[53];
    int x = 0;
	struct Contentor contentor;
    while((nread = read(fifoFD, buffer, sizeof(buffer))!=0)) {
        if (x == 0) {
            pthread_mutex_lock(&mutex360);
			int i = 0;
			char *divisao = strtok (buffer, " ");
			char *referencias [4];
			while(divisao != NULL){
				referencias[i++] = divisao;
				divisao = strtok (NULL, " ");
			}
			memcpy(contentor.numero_serie, referencias[0], 36);
			memcpy(contentor.porto_destino, referencias[1], 3);
            char entrada[11];
            memcpy(entrada, referencias[2], 10);
            char saida[2];
            memcpy(saida, referencias[3], 1);
            contentor.marca_tempo_entrada = atol(entrada);
            contentor.marca_tempo_saida = atol(saida);
            time_t timeNow = time(NULL);
			contentor.marca_tempo_saida = timeNow;
            char linha[100];
            sprintf(linha, "%s %s %ld %ld\n",contentor.numero_serie, contentor.porto_destino, contentor.marca_tempo_entrada, contentor.marca_tempo_saida);
			write(fileFD, linha, sizeof(linha));
            printf("%s", linha);
            x = 1;
            pthread_mutex_unlock(&mutex360);
        }
        else {
            x = 0;
        }
    }
    close(fifoFD);
	pthread_exit(0);
}