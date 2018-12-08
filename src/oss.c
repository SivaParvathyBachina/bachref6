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
int req,child_pids[18], child_count = 0, log_lines = 0, nextloc = 0, pf;
long seed_value;
Queue* blockedqueue;
FrameTable frames;
PageTable pages;
Queue* processq;
Queue* addressq;
Queue* timeq;
Queue* reqtypeq;

void clearSharedMemory() {
fprintf(stderr, "------------------------------- CLEAN UP ----------------------- \n");
shmdt((void *)clock);
fprintf(stderr,"Closing File \n");
fclose(logfile);
fprintf(stderr, "Child Forked %d \n", child_count); 
fprintf(stderr, "Faults Total Vs Requests %d ---- %d \n", pf, req);
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
	pf++;
	int r;
	while(pages.pagestatus[nextloc] == 1)
	{
		pages.pagestatus[nextloc] = 0;
		frames.reference[pages.page[nextloc]] = 0;
		nextloc = (nextloc+1)%32;
	}
		
	int firstIndex = pages.page[nextloc];
	if(firstIndex != -1)
	{
		frames.reference[firstIndex] = -1;
		frames.framestatus[firstIndex] = '.';
		frames.frame[firstIndex] = -1;	
	}

	//Found an empty page and adding memory value to that
	//0 for used, 1 for dirty, 2 for second chance
	pages.page[nextloc] = memory;
	frames.frame[memory] = nextloc;
	pages.pagestatus[nextloc] = 0;
	frames.reference[memory] = 0;
	nextloc = (nextloc+1) % 32;
	return nextloc;
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

	for(l = 0; l< 32; l++)
	fprintf(stderr, "%d ", pages.page[l]);
	fprintf(stderr, "\n");

	/*for(l = 0; l< 32; l++)
        fprintf(stderr, "%d ", pages.pagestatus[l]);
        fprintf(stderr, "\n");		
	*/
	
	for(l = 0; l<256; l++) 
	{
		//if(frames.framestatus[l] != '.')
		fprintf(stderr, "%c", frames.framestatus[l]);
	}
	fprintf(stderr, "\n");
	fprintf(stderr, "=====================================================================================\n");

}


int checkIfTimePassed()
{
	double timefirst = front_item(timeq);
	if(timefirst > 0)
	{
	float clocktime = clock -> nanoseconds + (clock -> seconds * NANOSECOND);
	double value = clocktime / 1000000; 
	if(timefirst > value )
	{
		int ele = dequeue(timeq);
		return 1;
	}
	}
	return -1;
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
	pages.pagestatus[m] = 0;
}

for(n = 0; n< 256; n++)
{
	frames.frame[n] = -1;
	frames.reference[n] = -1;
	frames.framestatus[n] = '.';

}
clock -> seconds = 0;
clock -> nanoseconds = 0;
processq = create_queue(20);
addressq = create_queue(20);
timeq = create_queue(20);
reqtypeq = create_queue(20);
pid_t mypid;
int nextspawntime;
int emptyloc = -1,b, next_child_time_nano, next_child_time_sec;
while(1)
{
		emptyloc = -1;
		for(b = 0; b< 18; b++)
		{
			if(child_pids[b] == 0)
			{
			emptyloc = b;
			break;
			}
		}
		
		if((emptyloc >= 0))
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
		child_pids[emptyloc] = mypid;
		}

		srand(seed_value++);
		nextspawntime += rand() % 4;

	if((msgrcv(msgqueueId,  &msgqueue, sizeof(msgqueue), 1, IPC_NOWAIT)) != -1)
	{
		int readorwrite = msgqueue.rwflag;
		if(readorwrite == 2)
                {
                int processId = msgqueue.processNumber;
                int pidloc = getIndexById(processId);
                fprintf(stderr, "Process pid %d Terminating \n", processId);
                child_pids[pidloc] = 0;
                waitpid(processId, &status, 0);
                }
		else
		{
		int frameId;
		req++;
		int processId = msgqueue.processNumber;
		int address = msgqueue.memreq;
                int address_val = address / 1000;
                int pageadd =  getpagetable(address_val);
		double clocknano = clock -> nanoseconds + (clock -> seconds * NANOSECOND);
		double incrementer = clocknano / 1000000;
		double total = 15 + incrementer;
                if(pageadd == -1)
		{
                int update = checkFrameAvailability(address_val);
		fprintf(stderr, "Blocking process %d request for %d address location \n", processId, address);
		enqueue(processq, processId);
		enqueue(addressq, address);
		enqueue(timeq,total);
		enqueue(reqtypeq, readorwrite);
                }
		else
                {
                        frameId = pages.page[pageadd];
                        frames.reference[address_val] = 1;
                        pages.pagestatus[pageadd] = 1;

		if(readorwrite == 0)         //Read Operation
		{	
		fprintf(stderr, "OSS: Process Id %d requesting Read %d \n", processId,address);
		frames.framestatus[address_val] = 'U';
		printData();
		msgqueue.msg_type = processId;
		msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
		}
		else 				// Write Operation
		{
		fprintf(stderr, "OSS: Process Id %d requesting Write %d \n", processId,address);
		frames.framestatus[address_val] = 'D';
		printData();
		msgqueue.msg_type = processId;
		msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
		}
		}
		clock -> nanoseconds += 10;
        	if(clock -> nanoseconds >= NANOSECOND)
        	{
        	clock -> seconds += (clock -> nanoseconds) / NANOSECOND;
        	clock -> nanoseconds = (clock -> nanoseconds) % NANOSECOND;
        	}
	}
	}
	
	else
	{
	clock -> nanoseconds += 100;
	if(clock -> nanoseconds >= NANOSECOND) 
	{
	clock -> seconds += (clock -> nanoseconds) / NANOSECOND;
	clock -> nanoseconds = (clock -> nanoseconds) % NANOSECOND;
	}
	}
	
		if(checkIfTimePassed() > 0)
                {
                        int pId = (int) dequeue(processq);
                        int addre = (int) dequeue(addressq);
			int rw = (int) dequeue(reqtypeq);
			fprintf(stderr, "Unblocking process %d request for %d address location \n", pId, addre);
			int uploc = checkFrameAvailability(addre / 1000);
			if(rw == 0)
			{
				fprintf(stderr, "OSS: Process Id %d finished Write %d \n", pId,addre);
	        	        frames.framestatus[addre / 1000] = 'D';
        	        	printData();
			}
			else
			{
				 fprintf(stderr, "OSS: Process Id %d finished Write %d \n", pId,addre);
                                frames.framestatus[addre/1000] = 'U';
                                printData();	
			}
			
                	msgqueue.msg_type = pId;
                	msgsnd(msgqueueId, &msgqueue, sizeof(msgqueue), 0);
		}
			
}
return 0;
}

