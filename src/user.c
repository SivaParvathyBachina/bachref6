#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <signal.h>
#include "shared_mem.h"
#include <fcntl.h>
#define MILLION 1000000
#define NANOSECOND 1000000000

int status,x,i,m,n,p,q;
int clockId, msgqueueId;
logical_clock *clock;
pid_t mypid;
long int startsec, startnano;
long seed_child = 0;

int main (int argc, char *argv[]) {

mypid = getpid();

while((x = getopt(argc,argv, "c:m:")) != -1)
switch(x)
{
case 'c': 
	clockId = atoi(optarg);
	break;
case 'm': 
	msgqueueId = atoi(optarg);
	break;
}

clock = (logical_clock*) shmat(clockId, NULL, 0);

if(clock == (void *) -1)
{
        perror("Error in attaching Clock User \n");
        exit(1);
}

int randomNumber, choice, request_count = 0;
while(1)
{
	if((clock -> seconds >= startsec) || (clock -> nanoseconds >= startnano))
	{	
	srand((seed_child++) * mypid);
	randomNumber = rand() % 100;

	if(randomNumber <= 70)
        	choice = 0;
	else
        	choice = 1;

	//fprintf(stderr, "Choice %d \n", choice);

	if(choice == 0)		// Read Operation
	{
		srand((seed_child++)  * mypid);
		int addressLoc = rand() % 256000;
		msgqueue.msg_type = 1;
		msgqueue.processNumber = mypid;
		msgqueue.memreq = addressLoc;
		msgqueue.rwflag = 0;
		msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
		msgrcv(msgqueueId, &msgqueue, sizeof(msgqueue), mypid, 0);
		request_count++;
	}
	else		// Write Operation
	{
		srand((seed_child++)  * mypid);
                int addressLoc = rand() % 256000;
                msgqueue.msg_type = 1;
                msgqueue.processNumber = mypid;
                msgqueue.memreq = addressLoc;
                msgqueue.rwflag = 1;
                msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
                msgrcv(msgqueueId, &msgqueue, sizeof(msgqueue), mypid, 0);
		request_count++;
	}
	
	if(request_count > 100)
	{
		msgqueue.msg_type = 1;
                msgqueue.processNumber = mypid;
                msgqueue.rwflag = 2;
		waitpid(mypid, &status, 0);
                msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
		break;
	
	} 
	srand((seed_child++)  * mypid);
	startnano += rand() % 1000;
	if(startnano >= NANOSECOND)
	{
		startsec += startnano/NANOSECOND;
		startnano = startnano % NANOSECOND;
	}
	}
}

return 0;
}
