#ifndef _SHARED_MEM_H_
#define _SHARED_MEM_H_

typedef struct
{
	long seconds;
	long nanoseconds;
}logical_clock;


struct msg_buf{
	long msg_type;
	int processNumber;
	int memreq;
	int rwflag;
}msgqueue;


typedef struct
{
	int page[32];	
	int pagestatus[32];
	int dirtybit[32];
}PageTable;

typedef struct {
	int frame[256];
	int framestatus[256];
	int reference[256];
}FrameTable;


#endif
