#include <stdio.h> 
#include <stdlib.h> 


typedef struct
{
    int front_item, rear_item, size;
    unsigned capacity;
    int* queue_array;
}Queue;

Queue* create_queue(unsigned capacity)
{
    Queue* priority_queue = (Queue*) malloc(sizeof(Queue));
    priority_queue->capacity = capacity;
    priority_queue->front_item = priority_queue->size = 0;
    priority_queue->rear_item = capacity - 1;
    priority_queue->queue_array = (int*) malloc(priority_queue->capacity * sizeof(int));
    return priority_queue;
}

int isFull( Queue* priority_queue)
{
	 return (priority_queue->size == priority_queue->capacity);  
}

int isEmpty( Queue* priority_queue)
{ 
	 return priority_queue->size; 
}

void enqueue( Queue* priority_queue, int push_item)
{
    if (isFull(priority_queue))
        return;
    priority_queue->rear_item = (priority_queue->rear_item + 1)%priority_queue->capacity;
    priority_queue->queue_array[priority_queue->rear_item] = push_item;
    priority_queue->size = priority_queue->size + 1;
}

int dequeue( Queue* priority_queue)
{
    if (isEmpty(priority_queue) == 0)
        return -1;

    int item = priority_queue->queue_array[priority_queue->front_item];
    priority_queue->front_item = (priority_queue->front_item + 1)%priority_queue->capacity;
    priority_queue->size = priority_queue->size - 1;
    return item;
}

int front_item(Queue* priority_queue)
{
    if (isEmpty(priority_queue) == 0)
        return -1;
    return priority_queue->queue_array[priority_queue->front_item];
}


int rear_item(Queue* priority_queue)
{
    if (isEmpty(priority_queue) == 0)
        return -1;
    return priority_queue->queue_array[priority_queue->rear_item];
}
