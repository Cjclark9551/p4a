#include "cs537.h"
#include "request.h"
#include "pthread.h"
#include "stdio.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// CS537: Parse the new arguments too
struct Queue
{
   int front, back, size, capacity;
   int* array;
};

struct Queue* queue;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill  = PTHREAD_COND_INITIALIZER;

int threads, buffers;

void queue_init(int capacity)
{
   queue = (struct Queue*)malloc(sizeof(struct Queue));
   queue->capacity = capacity;
   queue->front = 0;
   queue->back = capacity - 1;
   queue->size = 0;
   queue->array = (int*)malloc(capacity*sizeof(int));
}

int queue_full()
{
	if (queue->size == queue->capacity)
		return 1;
	else
		return 0;
}

int queue_empty() 
{
	if(queue->size == 0)
		return 1;
	else
		return 0;
}
void queue_put(int n_thread)
{
   printf("queue put");
   if(!(queue->size == queue->capacity))
   {
      queue->back = (queue->back+1)%queue->capacity;
      queue->array[queue->back] = n_thread;
      queue->size = queue->size + 1;
   }else{
      printf("Queue is full");
   }

}

int get()
{
   if(queue->size != 0)
   {
      int n_thread = queue->array[queue->front];
      queue->size = queue->size - 1;
      queue->front = (queue->front + 1)% queue->capacity;
      return n_thread;
   }else{
      printf("Queue is empty");
      return -1;
   }
}

void getargs(int *port, int *threads, int *buffers, int argc, char *argv[])
{
    if (argc != 4) {
	fprintf(stderr, "Usage: %s <port> <threads> <buffers>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *buffers = atoi(argv[3]);
}

void *producer(int connfd)
{
	for(;;) {
		pthread_mutex_lock(&mutex);
		
		while(queue_full() == 1) {
			pthread_cond_wait(&empty, &mutex);
		}

		queue_put(connfd);

		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&mutex);	
	}
}

void *consumer()
{
   while(1) 
   {
      printf("consumer\n");
      pthread_mutex_lock(&mutex);
      while(queue_empty() == 1)
      {
         pthread_cond_wait(&fill, &mutex);
      }
      //Do something with item at start of queue
      int connfd = get();
      printf("connfd in consumer: %d", connfd);
      requestHandle(connfd); 
      Close(connfd);      

      pthread_cond_signal(&empty);
      pthread_mutex_unlock(&mutex);
   }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    getargs(&port, &threads, &buffers, argc, argv);
    queue_init(buffers);
    
    // 
    // CS537: Create some threads...
    //

    pthread_t workers[threads];
    for(int workerNum = 0; workerNum < threads; workerNum++)
    {
	pthread_create(&workers[workerNum], NULL, consumer, NULL);
    }

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// CS537: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 

	producer(connfd);
//	requestHandle(connfd);

//	Close(connfd);
    }

}
