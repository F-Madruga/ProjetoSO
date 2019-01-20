#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <uuid/uuid.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define THREADS 3
#define NUMPORTOS 10

char *portos[NUMPORTOS] = {"ANR", "BUS", "DXB", "GUA", "HAM", "HKG", "LAX", "RTM", "SHA", "SIN"};

void *writeFile(void* file);
char *getuuid();

struct FileData
{
	int numLines;
	char fileName[15];
};

int main(int argc, char const *argv[])
{
	if(argc != 2){
		printf("Usage: %s <numero de linhas> \n" ,argv[0]);
		exit(-1);
	}
	
	time_t t;
	srand((unsigned)time(&t));

	pthread_t threadsId[THREADS];
	struct FileData file[THREADS];

	//Criação das threads
	for (int i = 0; i < THREADS; i++) {
		file[i].numLines = atoi(argv[1]);
		sprintf(file[i].fileName,"input%d.dat", i);
		pthread_create(&threadsId[i], NULL, writeFile, (void *)&file[i]);
	}

	//Espera das threads
	for (int i = 0; i < THREADS; i++) {
		pthread_join(threadsId[i], NULL);
	}
	return 0;
}

void *writeFile(void* file) {
	struct FileData fileData = *(struct FileData *)file;

	//Apaga o ficheiro caso ele já exista
	if (access(fileData.fileName, F_OK) != -1 ) {
		remove(fileData.fileName);
	}
	//Cria e abre o ficheiro
	int fd;
	fd = open(fileData.fileName, O_WRONLY | O_CREAT, 0666);
	for (int i = 0; i < fileData.numLines; i++) {
		char linha[41];
		sprintf(linha, "%s %s\n", getuuid(), portos[rand() % NUMPORTOS]);
		write(fd, linha, sizeof(linha));
	}
	close(fd);
	pthread_exit(0);
}

char *getuuid() {
	static char str[36]; 
	uuid_t uuid;
	uuid_clear(uuid);
	uuid_generate_random(uuid);
	uuid_unparse(uuid,str); 
	return str; 
}