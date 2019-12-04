#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#define NANO 1000000000
#define MY_PULSE_CODE _PULSE_CODE_MINAVAIL
#define PAUSE_PULSE 1
#define QUIT_PULSE 2
typedef union{
	struct _pulse pulse;
	char msg[128];
}my_message_t;
my_message_t            msg;
typedef struct DataTable {
	int TST;
	int TSB;
	int NIB;
	char pattern[13];
} DataTableRow;

int checkInput(int TST, int TSB);
int argCheck(int arg);
//volatile int MY_PULSE_CODE;
DataTableRow t[] = {
		{2,4,4,"|1&2&"},
		{3,4,6,"|1&2&3&"},
		{4,4,8,"|1&2&3&4&"},
		{5,4,10,"|1&2&3&4-5-"},
		{3,8,6,"|1-2-3-"},
		{6,8,6,"|1&a2&a"},
		{9,8,9,"|1&a2&a3&a"},
		{12,8,12,"|1&a2&a3&a4&a"}};

int patternIndex;
float SPI;
int timerVal;
float SPM;
float BPM;
float TST;
float TSB;
char data[255];
int server_coid;
///////////////////THREAD//////////////////////////
void* metronome_thread(void* arg){
	struct sigevent         event;
	struct itimerspec       itime;
	timer_t                 timer_id;
	//int                     chid;
	int                     rcvid;
	name_attach_t *attach;
	attach = name_attach(NULL, "metronome",0);
	if(attach == NULL){
		perror("Name attach failed");
		return EXIT_FAILURE;
	}
	//chid = ChannelCreate(0);


	event.sigev_notify = SIGEV_PULSE;

	event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0,
			attach->chid,
			_NTO_SIDE_CHANNEL, 0);

	event.sigev_priority = SIGEV_PULSE_PRIO_INHERIT;

	event.sigev_code = MY_PULSE_CODE;
	timer_create(CLOCK_REALTIME, &event, &timer_id);
	//printf("%f\n",SPI);
	/* 500 million nsecs = .5 secs */
	itime.it_value.tv_nsec = SPI*NANO;
	itime.it_interval.tv_sec = 0;//time until timer resets
	/* 500 million nsecs = .5 secs */
	itime.it_interval.tv_nsec = SPI*NANO;
	timer_settime(timer_id, 0, &itime, NULL);


	int i = 0;
	int value;

	for (;;) {
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);
		if (rcvid == 0) { /* we got a pulse */
			//printf("%d",msg.pulse.code);
			switch(msg.pulse.code){
			case MY_PULSE_CODE: {
				if(i > strlen(t[patternIndex].pattern)){
					i = 0;
					printf("\n");
				}else{

					if(i == 0){
						printf("%c",t[patternIndex].pattern[0]);
						printf("%c",t[patternIndex].pattern[1]);
						i++;
					}else{
						printf("%c",t[patternIndex].pattern[i]);
					}
					fflush(stdout);
					i++;
				}
			break;
			}
			case PAUSE_PULSE:

			value = msg.pulse.value.sival_int;
			printf("<Pausing %d>", value);
			fflush(stdout);
			itime.it_interval.tv_nsec = 0;
			timer_settime(timer_id, 0, &itime, NULL);
			delay(value*1000);
			itime.it_interval.tv_nsec = SPI*NANO;
			timer_settime(timer_id,0, &itime, NULL);
			break;
			case QUIT_PULSE: name_detach(attach,0);return NULL;
			}

		} /* else other messages ... */

	}

	// Phase III
	ConnectDetach( event.sigev_coid );
	//ChannelDestroy( chid );
	//	printf("%c", t[patternIndex].pattern[i]);
	//printf("%d\n",timerVal);



	return NULL;
}
///////////////////THREAD//////////////////////////

int io_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb) {
	int nb;
	nb = strlen(data);

	//test to see if we have already sent the whole message.
	if (ocb->offset == nb)
		return 0;
	//	printf("[Metronome: %.0f beats/min, time signature %.0f/%.0f, sec-per-interval: %.2f, nanoSecs: %d]\n",BPM,TST,TSB,SPI, timerVal);

	//We will return which ever is smaller the size of our data or the size of the buffer
	nb = min(nb, msg->i.nbytes);

	//Set the number of bytes we will return
	_IO_SET_READ_NBYTES(ctp, nb);

	//Copy data into reply buffer.
	SETIOV(ctp->iov, data, nb);

	//update offset into our data used to determine start position for next read.
	ocb->offset += nb;

	//If we are going to send any bytes update the access time for this resource.
	if (nb > 0)
		ocb->attr->flags |= IOFUNC_ATTR_ATIME;

	return (_RESMGR_NPARTS(1));
}
int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb) {
	int nb = 0;

	if (msg->i.nbytes == ctp->info.msglen - (ctp->offset + sizeof(*msg))) {
		/* have all the data */

		char *buf;
		char *alert_msg;
		int i, small_integer;
		buf = (char *) (msg + 1);

		if (strstr(buf, "pause") != NULL) {

			for (i = 0; i < 2; i++) {
				alert_msg = strsep(&buf, " ");
			}
			//////ALSO ADD A CHECK FOR QUIT MESSAGE///////
			small_integer = atoi(alert_msg);
			if (small_integer >= 1 && small_integer <= 99) {
				//FIXME :: replace getprio() with SchedGet()
				//printf("\nSending pulse\n");
				MsgSendPulse(server_coid, SchedGet(0, 0, NULL),
						PAUSE_PULSE, small_integer);
			} else {
				printf("Integer is not between 1 and 99.\n");
			}
		}else if(strstr(buf, "quit") != NULL){
			MsgSendPulse(server_coid, SchedGet(0, 0, NULL),QUIT_PULSE, 0);
		}

		else {
			printf("Invalid input\n");
			strcpy(data, buf);
		}

		nb = msg->i.nbytes;
	}
	_IO_SET_WRITE_NBYTES(ctp, nb);

	if (msg->i.nbytes > 0)

		return (_RESMGR_NPARTS(0));
}
int io_open(resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle,
		void *extra) {
	if ((server_coid = name_open("metronome", 0)) == -1) {
		perror("name_open failed.");
		return EXIT_FAILURE;
	}
	return (iofunc_open_default(ctp, msg, handle, extra));
}





int main(int argc, char* argv[]) {
	if (argCheck(argc)) {
		fprintf(stderr, "Invalid number of argument\n");
		return EXIT_FAILURE;
	}
	pthread_attr_t attr;
	BPM = atoi(argv[1]);
	TST = atoi(argv[2]);
	TSB = atoi(argv[3]);
	SPM = (60/BPM)*TST;

	patternIndex = checkInput(TST, TSB);
	if(patternIndex<0){
		fprintf(stderr, "Error, invalid time signatures\n");
		return EXIT_FAILURE;
	}
	SPI = (SPM/t[patternIndex].NIB);
	timerVal = SPI*NANO;

	dispatch_t* dpp;
	resmgr_io_funcs_t io_funcs;	resmgr_connect_funcs_t connect_funcs;
	iofunc_attr_t ioattr;
	dispatch_context_t *ctp;
	int id;
	snprintf(data, 255,"[Metronome: %.0f beats/min, time signature %.0f/%.0f, sec-per-interval: %.2f, nanoSecs: %d]\n",BPM,TST,TSB,SPI, timerVal );
	if ((dpp = dispatch_create()) == NULL) {
		fprintf(stderr, "%s:  Unable to allocate dispatch context.\n", argv[0]);
		return (EXIT_FAILURE);
	}
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs, _RESMGR_IO_NFUNCS,
			&io_funcs);

	connect_funcs.open = io_open;
	io_funcs.read = io_read;
	io_funcs.write = io_write;

	iofunc_attr_init(&ioattr, S_IFCHR | 0666, NULL, NULL);

	if ((id = resmgr_attach(dpp, NULL, "/dev/local/metronome", _FTYPE_ANY, 0,
			&connect_funcs, &io_funcs, &ioattr)) == -1) {
		fprintf(stderr, "%s:  Unable to attach name.\n", argv[0]);
		return (EXIT_FAILURE);
	}

	ctp = dispatch_context_alloc(dpp);


	pthread_attr_init(&attr);
	if(pthread_create(NULL, &attr, &metronome_thread, NULL)== -1){
		fprintf(stderr, "Error creating thread :(\n");
		return EXIT_FAILURE;
	}
	pthread_attr_destroy(&attr);
	while (1) {
		ctp = dispatch_block(ctp);
		dispatch_handler(ctp);
		if(msg.pulse.code == QUIT_PULSE){
			break;
		}
	}

	printf("\nExiting\n");


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
