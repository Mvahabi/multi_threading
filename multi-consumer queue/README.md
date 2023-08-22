# Assignment 3 directory: queue

Queue Implementation using Semaphores
This is a README file that provides an overview of a queue implementation using semaphores. The code provided demonstrates how to create a queue data structure with thread-safe operations using semaphores for synchronization.

## Implementation Details

The queue implementation consists of the following components:

### queue_t Structure: 
The main data structure representing the queue. It contains the necessary fields to maintain the queue, including the size, buffer (bb), semaphores for synchronization (full, empty, and mut), and indices for the head and tail.

### queue_new(int size): 
This function creates a new queue with the specified size. It dynamically allocates memory for the queue_t structure and initializes the necessary fields, including the semaphores. The function returns a pointer to the newly created queue.

### queue_delete(queue_t q): 
This function deletes an existing queue and frees the allocated memory. It also destroys the semaphores and sets the queue pointer to NULL.

### queue_push(queue_t *q, void *elem): 
This function adds an element to the queue. It uses semaphores to ensure mutual exclusion and proper synchronization between producers and consumers. It waits on the full semaphore to ensure that the queue is not full before adding the element. After adding the element, it updates the tail index and counter, and then signals the empty semaphore to notify consumers.

### queue_pop(queue_t *q, void elem): 
This function removes an element from the queue. It also uses semaphores for synchronization. It waits on the empty semaphore to ensure that the queue is not empty before removing the element. After removing the element, it updates the head index and counter, and then signals the full semaphore to notify producers.

## Usage

To use the queue implementation, follow these steps:

Include the necessary headers and define the queue structure.

Use the queue_new() function to create a new queue, providing the desired size as an argument.

Use queue_push() to add elements to the queue.

Use queue_pop() to remove elements from the queue.

When finished, use queue_delete() to free the allocated memory and destroy the queue.
