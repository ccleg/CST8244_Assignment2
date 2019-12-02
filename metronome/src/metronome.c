#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define NANO 1000000000
typedef struct DataTable {
	int TST;
	int TSB;
	int NIB;
	char pattern[13];
} DataTableRow;

int checkInput(int TST, int TSB);
int argCheck(int arg);
void* metronome_thread(void* arg){



}

DataTableRow t[] = {
		{2,4,4,"|1&2&"},
		{3,4,6,"|1&2&3&"},
		{4,4,8,"|1&2&3&4&"},
		{5,4,10,"|1&2&3&4-5-"},
		{3,8,6,"|1-2-3-"},
		{6,8,6,"|1&a2&a"},
		{9,8,9,"|1&a2&a3&a"},
		{12,8,12,"|1&a2&a3&a4&a"}};

int main(int argc, char* argv[]) {

	if (argCheck(argc)) {
		fprintf(stderr, "Invalid number of argument\n");
		return EXIT_FAILURE;
	}
	pthread_attr_t attr;
	int patternIndex;
	float BPM = atoi(argv[1]);
	float TST = atoi(argv[2]);
	float TSB = atoi(argv[3]);
	float SPM = (60/BPM)*TST;
	float SPI;
	int timerVal;

	patternIndex = checkInput(TST, TSB);
	if(patternIndex<0){
		fprintf(stderr, "Error, invalid time signatures\n");
		return EXIT_FAILURE;
	}
	for(int i = 0; i < strlen(t[patternIndex].pattern); i ++){
		//printf("%c", t[patternIndex].pattern[i]);
		 SPI = (SPM/t[patternIndex].NIB);
	}
	timerVal = SPI*NANO;
	printf("[Metronome: %.0f beats/min, time signature %.0f/%.0f, sec-per-interval: %.2f, nanoSecs: %d]",BPM,TST,TSB,SPI, timerVal);

	pthread_attr_init(&attr);
	pthread_create(NULL, &attr, &metronome_thread, NULL);
	pthread_attr_destroy(&attr);




	return EXIT_SUCCESS;
}
int checkInput(int TST, int TSB){
 for(int i = 0; i < 8; i++){
	 if(TST == t[i].TST && TSB == t[i].TSB){
		 return i;
	 }
 }
 return -1;
}
int argCheck(int arg) {
	if (arg < 3) {
		return 1;
	} else {
		return 0;
	}
}
