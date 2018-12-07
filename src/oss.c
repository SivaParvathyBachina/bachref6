#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include "shared_mem.h"
#include <sys/msg.h>
#include "priority_queue.h"
#define NANOSECOND 1000000000
#define LOGFILESIZE 100000

int status,x,i,p,q, clockId, msgqueueId;;
key_t clockKey, msgqueueKey;
logical_clock *clock;
FILE *logfile;
char *file_name;
int child_pids[18], child_count = 0, log_lines = 0, index = 0;
long seed_value;
Queue* blockedqueue;
FrameTable frames;
PageTable pages;

void clearSharedMemory() {
fprintf(stderr, "------------------------------- CLEAN UP ----------------------- \n");
shmdt((void *)clock);
fprintf(stderr,"Closing File \n");
fclose(logfile);
fprintf(stderr, "Child Forked %d \n", child_count); 
fprintf(stderr, "OSS started detaching all the shared memory segments \n");
shmctl(clockId, IPC_RMID, NULL);
msgctl(msgqueueId, IPC_RMID, NULL);
fprintf(stderr, "OSS Cleared the Shared Memory \n");
}

void killExistingChildren(){
int k;
for(k=0; k<18; k++)
{
if(child_pids[k] > 0)
{
kill(child_pids[k], SIGTERM);
}
}
}

int checkFrameAvailability(int memory)
{
	//framestatus = 1; for occupied 
	//framestatus = -1 unoccupied
	int r;
	while(pages.pagestatus[index] == 1)
	{
		pages.pagestatus[index] = 0;
		frames.reference[pages.page[index]] = 1;
		index = (index+1)%32;
	}
		
	int firstIndex = pages.page[index];
	if(firstIndex != -1)
	{
		frames.reference[firstIndex] = -1;
		frames.framestatus[firstIndex] = -1;
		frames.frame[firstIndex] = 5;	
	}

	//Found an empty page and adding memory value to that
	//0 for used, 1 for dirty, 2 for second chance
	pages.page[index] = memory;
	frames.frame[memory] = index;
	pages.pagestatus[index] = 0;
	frames.reference[memory] = 0;
	index = (index+1) % 32;
	return index;
}


int getIndexById(int processId)
{
	int l;
	for(l = 0; l < 18; l++)
	{
		if(child_pids[l] == processId)
		return l;
	}
}

int getpagetable(int memory)
{
	for(p = 0; p<32; p++)
	{
		if(pages.page[p] == memory)
		return p;
	}
	return -1;
}

void printData()
{
	fprintf(stderr, "Current Memory layout at time is \n");
	int l;
	fprintf(stderr, "Frame Layout \n");

	for(l = 0; l<256; l++) 
	{
	if(frames.reference[l] == -1)
		fprintf(stderr, ".");
	else if(frames.reference[l] == 0)
		fprintf(stderr, "U");
	else
		fprintf(stderr, "D");
	
	}
	frprintf(stderr, "\n");
	frprintf(stderr, "=====================================================================================\n");

}

void myhandler(int s) {
if(s == SIGALRM)
{
fprintf(stderr, "Master Time Done\n");
killExistingChildren();
clearSharedMemory();
}

if(s == SIGINT)
{
fprintf(stderr, "Caught Ctrl + C Signal \n");
fprintf(stderr, "Killing the Existing Children in the system \n");
killExistingChildren();
clearSharedMemory();
}
exit(1);
}

int main (int argc, char *argv[]) {
seed_value = time(NULL);
while((x = getopt(argc,argv, "hs:l:")) != -1)
switch(x)
{
case 'h':
        fprintf(stderr, "Usage: %s -l logfile_name  -h [help]\n", argv[0]);
        return 1;
case 'l':
	file_name = optarg;
	break;
case '?':
        fprintf(stderr, "Please give '-h' for help to see valid arguments \n");
        return 1;
}

blockedqueue = create_queue(20);
signal(SIGALRM, myhandler);
alarm(2);
signal(SIGINT, myhandler);

clockKey = ftok(".", 'c');
clockId = shmget(clockId, sizeof(logical_clock), IPC_CREAT | 0666);
if(clockId <0 )
{
	fprintf(stderr, "Error in shmget for Clock \n");
	exit(1);
}

clock = (logical_clock*) shmat(clockId, NULL, 0);
fprintf(stderr, "Allocated Shared Memory For OSS Clock \n");

msgqueueKey = ftok(".", 'p');
msgqueueId = msgget(msgqueueKey, IPC_CREAT | 0666);
if(file_name == NULL)
file_name = "default";
logfile = fopen(file_name, "w");

fprintf(stderr, "Opened Log File for writing Output::: %s \n", file_name);

int m, n;
for(m = 0; m < 32; m++)
{
	pages.page[m] = -1;
	pages.pagestatus[m] = -1;
}

for(n = 0; n< 256; n++)
{
	frames.frame[n] = -1;
	frames.reference[n] = -1;
	frames.framestatus[n] = -1;

}
clock -> seconds = 0;
clock -> nanoseconds = 0;

pid_t mypid;
int nextspawntime;
int index = -1,b, next_child_time_nano, next_child_time_sec;
while(1)
{
		index = -1;
		for(b = 0; b< 18; b++)
		{
			if(child_pids[b] == 0)
			{
			index = b;
			break;
			}
		}
		
		if((index >= 0))
		{
			if((mypid = fork()) ==0)
			{
			char argument2[20],argument3[50], argument4[4], argument5[10];
                	char *clock_val = "-c";
			char *msg_val = "-m";
			char *arguments[] = {NULL,clock_val,argument2,msg_val, argument3, NULL};	
			arguments[0]="./user";
			sprintf(arguments[2], "%d", clockId);
			sprintf(arguments[4], "%d", msgqueueId);
			execv("./user", arguments);
                	fprintf(stderr, "Error in exec");
			exit(0); 
			}
		child_count++;
		child_pids[index] = mypid;
		}

		srand(seed_value++);
		nextspawntime += rand() % 4;

	if((msgrcv(msgqueueId,  &msgqueue, sizeof(msgqueue), 1, IPC_NOWAIT)) != -1)
	{
	if(msgqueue.rwflag == 0)         //Read Operation
	{
		int processId = msgqueue.processNumber;
		int index = getIndexById(processId);
		int address = msgqueue.memreq;
		int adrress_val = address / 1000;
		//int pageadd =  getpagetable(address_val);	
		//if(pageadd == -1)
		//int update = frameNotAvaialable(adrress_val);				
		fprintf(stderr, "OSS: Process Id %d requesting Read %d \n", processId,address);	
		msgqueue.msg_type = processId;
		msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
	}
	else if(msgqueue.rwflag == 1)	// Write Operation
	{
		int processId = msgqueue.processNumber;
                int index = getIndexById(processId);
                int address = msgqueue.memreq;
		fprintf(stderr, "OSS: Process Id %d requesting Write %d \n", processId,address);
		msgqueue.msg_type = processId;
		msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
	}
	else
	{
		int processId = msgqueue.processNumber;
                int index = getIndexById(processId);
		fprintf(stderr, "Process pid %d Terminating \n", processId);
		child_pids[index] = 0;
		waitpid(processId, &status, 0);
	}
	}
	
	clock -> nanoseconds += 1000;
	if(clock -> nanoseconds >= NANOSECOND) 
	{
	clock -> seconds += (clock -> nanoseconds) / NANOSECOND;
	clock -> nanoseconds = (clock -> nanoseconds) % NANOSECOND;
	}
}
return 0;
}

