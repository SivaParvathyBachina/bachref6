#include <stdio.h> 
#include <stdlib.h> 


typedef struct
{
    double front_item, rear_item;
  	int size;
    double capacity;
    double* queue_array;
}Queue;

Queue* create_queue(unsigned capacity)
{
    Queue* priority_queue = (Queue*) malloc(sizeof(Queue));
    priority_queue->capacity = capacity;
    priority_queue->front_item = priority_queue->size = 0;
    priority_queue->rear_item = (double) capacity - 1;
    priority_queue->queue_array = (double*) malloc(priority_queue->capacity * sizeof(double));
    return priority_queue;
}

int isFull( Queue* priority_queue)
{
	 return (priority_queue->size == priority_queue->capacity);  
}

int isEmpty(Queue* priority_queue)
{ 
	 return priority_queue->size; 
}

void enqueue(Queue* priority_queue, double push_item)
{
    if (isFull(priority_queue))
        return;
    
    priority_queue->rear_item = (int) (priority_queue->rear_item + 1)% (int) 
    priority_queue->capacity;
    priority_queue->queue_array[(int) priority_queue->rear_item] = push_item;
    priority_queue->size = priority_queue->size + 1;
  
}

double dequeue( Queue* priority_queue)
{
    if (isEmpty(priority_queue) == 0)
        return -1;

    double item = (int) priority_queue->queue_array[(int)priority_queue->front_item];
    priority_queue->front_item = (int)(priority_queue->front_item + 1)% (int)priority_queue->capacity;
    priority_queue->size = priority_queue->size - 1;
    return item;
}

double front_item(Queue* priority_queue)
{
    if (isEmpty(priority_queue) == 0)
        return -1;
    return priority_queue->queue_array[(int)priority_queue->front_item];
}


double rear_item(Queue* priority_queue)
{
    if (isEmpty(priority_queue) == 0)
        return -1;
    return priority_queue->queue_array[(int)priority_queue->rear_item];
}
