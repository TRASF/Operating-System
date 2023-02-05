#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "common.h"
#include "common_threads.h"
#ifdef linux
#include <semaphore.h>
#elif __APPLE__
#include "zemaphore.h"
#endif

int BUFFERSIZE = 32;


int *buffer;
int processes;
int devices;
int totalReq;
int min_time = 100;
int max_time = 500;

int action = 1;

int count  = 0;
int sumWait = 0;
int n = 0;
int totalRun = 0;
int out = 0, in = 0;
int err;
sem_t empty;
sem_t full;

pthread_mutex_t mutex;
pthread_cond_t cons;
pthread_cond_t pros;

int process(/*char c, void *arg*/){
    float r = (float) rand() / (float) RAND_MAX;
    int random_wait_time = (int) (r * (500 - 100) + 100);
    // if(c == 'c') printf("-Device: %lld Sleep for %d \n", (long long int) arg, random_wait_time);
    // if(c == 'p') printf("-Process: %lld Sleep for %d \n", (long long int) arg, random_wait_time);
    // if(c == 'c') printf("-Device: %lld Wake up now\n", (long long int) arg);
    // if(c == 'p') printf("-Process: %lld Wake up now\n", (long long int) arg);
    return random_wait_time;
}


void do_fill(int value) {
    buffer[count] = value;
    printf("-----Insert Item %d at %d\n",  value, count);
    count++;
    // if (count == BUFFERSIZE) count = 0;
}

int do_get() {
    int tmp = buffer[count];
    printf("-----remove %d from %d\n", tmp, count);
    count--;
    // if (count == BUFFERSIZE) count = 0;
    return tmp;
}

void* producer(void *arg) {
    int i, wait;
    switch (action){
        case 1: 
            for (i = 0; i < totalReq; i++) {
                wait = process();
                usleep(wait);
                Sem_wait(&empty);
                Mutex_lock(&mutex);
                do_fill(i);
                in = wait;
                Mutex_unlock(&mutex);
                Sem_post(&full);              
                totalRun++;              
            }
            // // end case
            for (i = 0; i < devices; i++) {
                Sem_wait(&empty);
                Mutex_lock(&mutex);
                do_fill(-1);
                Mutex_unlock(&mutex);
                Sem_post(&full); 
            }
                             
            break;
        case 2:
            break;
        case 3:
            break;
    }


    return NULL;
}
                                                                               
void* consumer(void *arg) {
    int tmp;
    float r = (float) rand() / (float) RAND_MAX;
    int random_wait_time = (int) (r * (max_time - min_time) + min_time);
        while(tmp != -1){
            Sem_wait(&full);
            Mutex_lock(&mutex);
            tmp = do_get();
            err = tmp;
            out = random_wait_time;
            Mutex_unlock(&mutex);
            Sem_post(&empty);
            usleep(random_wait_time);
        }

    totalRun++;    

    return NULL;
// }
// while(1){

//                 Sem_wait(&full);
//                 Mutex_lock(&mutex);
//                 while(count == 0){
//                     Cond_wait(&condl, &mutex);
//                 }
//                 if(count > 0){
//                     tmp = buffer[use];
//                     use = (use + 1) % BUFFERSIZE;
//                     count--;
//                 }
                
//                 if(tmp == -1) printf("Remove end case\n");
//                 Cond_signal(&condl);
//                 Mutex_unlock(&mutex);
//                 Sem_post(&empty);
                
//                 usleep(random_wait_time);
//                 totalRun++;
//                 sumWait += random_wait_time;
//                 printf("---remove %d from %d\n", tmp, use);
//             }   
        // }
        // while(1){

        //         Sem_wait(&full);
        //         Mutex_lock(&mutex);
        //         while(count == 0){
        //             Cond_wait(&condl, &mutex);
        //         }
        //         if(count > 0){
        //             tmp = buffer[use];
        //             use = (use + 1) % BUFFERSIZE;
        //             count--;
        //         }
                
        //         if(tmp == -1) printf("Remove end case\n");
        //         Cond_signal(&condl);
        //         Mutex_unlock(&mutex);
        //         Sem_post(&empty);
                
        //         usleep(random_wait_time);
        //         totalRun++;
        //         sumWait += random_wait_time;
        //         printf("---remove %d from %d\n", tmp, use);
        //     }   
        // }

int main(int argc, char *argv[]) {
    if (argc != 7) {
	    fprintf(stderr, "usage: %s <Processes> <Devices> <Total Request> <Minimum time> <Maximum time> <Action when Full>\n", argv[0]);
	exit(1);
    }
    processes = atoi(argv[1]);
    devices = atoi(argv[2]);
    totalReq = atoi(argv[3]);
    min_time = atoi(argv[4]);
    max_time = atoi(argv[5]);
    action  = atoi(argv[6]);

    buffer = (int *) malloc(BUFFERSIZE * sizeof(int));
    assert(buffer != NULL);
    int i;
    for (i = 0; i < BUFFERSIZE; i++) {
	    buffer[i] = 0;
    }

    Sem_init(&empty, BUFFERSIZE);
    Sem_init(&full, 0);
    Mutex_init(&mutex);   
    
    pthread_t pid[processes], cid[devices];
    for(int i = 0; i < processes; i++){
        Pthread_create(&pid[i], NULL, &producer, &i);
        printf("-Create process: %d\n", i);
    }
    
    for (i = 0; i < devices; i++) {
	    Pthread_create(&cid[i], NULL, &consumer, &i); 
        printf("-Create device: %d\n", i);
    }

    
    
    for(int i = 0; i < processes; i++){
        printf("-Join process: %d\n", i);
        Pthread_join(pid[i], NULL); 
    } 
    for (i = 0; i < devices; i++) {
        printf("-Join device: %d\n", i);
	    Pthread_join(cid[i], NULL); 
    }
    
    // exit(1);
    // printf("Sum Wait Time: %d | Number of task: %d | Average Wait Time :\n", wait, count);
    printf("Total Runtime: %d times \nAverage wait time:\n", totalRun);
    pthread_mutex_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    
    return 0;
}



