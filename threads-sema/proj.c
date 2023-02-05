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

int BUFFERSIZE = 120;


int *buffer;
int processes;
int devices;
int totalReq;
int min_time = 100;
int max_time = 500;

int action = 1;
int fill = 0, use = 0;

int count  = 0;
int sumWait = 0;
int n = 0;
int totalRun = 0;
int out = 0, in = 0;
int err = 0;
int drop = 0;
sem_t empty;
sem_t full;
pthread_mutex_t mutex;
pthread_cond_t cond;

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
    buffer[fill] = value;
    fill = (fill + 1) % BUFFERSIZE;
    count++;
}

int do_get() {
    int tmp = buffer[use];
    buffer[use] = -2;
    use = (use + 1) % BUFFERSIZE;
    count--;
    return tmp;
}

void fill_drop(int value) {
    if(buffer[fill] < 0){
       buffer[fill] = value;
        fill = (fill + 1) % BUFFERSIZE;
        count++; 
    }
    if(buffer[fill] > 0) drop++;
    
}

int replace(int *idx)
{
    int old, new, tmp;
    do
    {
        old = *idx;
        new = (old + 1) % BUFFERSIZE;
        tmp = buffer[old];
        buffer[old] = buffer[new];
        buffer[new] = tmp;
    } while(!__sync_bool_compare_and_swap(idx, old, new) && count > new);
    // printf("Replaced %d to %d\n", buffer[old], buffer[new]);
    return old;
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
                while((fill + 1) % BUFFERSIZE == use) Cond_wait(&cond, &mutex);
                do_fill(i);
                Cond_signal(&cond);
                Mutex_unlock(&mutex);
                Sem_post(&full);     
                totalRun++;
                sumWait += wait;           
            } 
         
            break;
        case 2:
            for (i = 0; i < totalReq; i++) {
                wait = process();
                usleep(wait);
                Sem_wait(&empty);
                Mutex_lock(&mutex);
                while((fill + 1) % BUFFERSIZE == use) Cond_wait(&cond, &mutex);
                fill_drop(i);    
                Cond_signal(&cond);
                Mutex_unlock(&mutex);
                Sem_post(&full);     
                totalRun++;
                sumWait += wait;    
                      
            } 
         printf("Drop %d\n", drop);
                 
            break;
        case 3:
            for(int i = 0; i < totalReq; i++){
                do_fill(i);
                wait = process();
                usleep(wait);
                while(1){
                    Mutex_lock(&mutex);
                    if((fill + 1) % BUFFERSIZE != use) break; 
                    Mutex_unlock(&mutex);
                    usleep(wait);
                 }
                replace((int*) &fill);
                Mutex_unlock(&mutex);
            }
            break;
    }                
            for (i = 0; i < devices; i++) {
                wait = process();
                usleep(wait);
                Sem_wait(&empty);
                Mutex_lock(&mutex);
                while((fill + 1) % BUFFERSIZE == use) Cond_wait(&cond, &mutex);
                do_fill(-1);
                Cond_signal(&cond);
                Mutex_unlock(&mutex);
                Sem_post(&full);     
                // totalRun++;
                // sumWait += wait;             
            }     
            printf("--Producer: %lld Finished produced\n", (long long int) arg);    
        

    return NULL;
}
                                                                               
void* consumer(void *arg) {
    int tmp;
    float r = (float) rand() / (float) RAND_MAX;
    int random_wait_time = (int) (r * (max_time - min_time) + min_time);
            while(tmp != -1){
                Sem_wait(&full);
                Mutex_lock(&mutex);
                while(fill == use) Cond_wait(&cond, &mutex);
                tmp = do_get();
                Mutex_unlock(&mutex);
                Cond_signal(&cond);
                Sem_post(&empty);
                usleep(random_wait_time);
            }            
            totalRun++;
            sumWait += random_wait_time;
            printf("--Consumer %lld Finished comsumed\n", (long long int) arg);
    return NULL;
}


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
	    buffer[i] = -2;
    }

    Sem_init(&empty, BUFFERSIZE);
    Sem_init(&full, 0);
    Mutex_init(&mutex);
    Cond_init(&cond);
    pthread_t pid[processes], cid[devices];
    for(int i = 0; i < processes; i++){
        Pthread_create(&pid[i], NULL, &producer, (void*) (long) i);
        printf("-Create process: %d\n", i);
    }
    
    for (i = 0; i < devices; i++) {
	    Pthread_create(&cid[i], NULL, &consumer, (void*) (long) i); 
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
    
    
    // printf("Sum Wait Time: %d | Number of task: %d | Average Wait Time :\n", wait, count);
    switch (action)
    {
    case 1:
    printf("Total  Waittimes: %d \nTotal Runtime: %d \nAverage wait time: %d\n", sumWait, totalRun, (sumWait / totalRun));
        break;
     case 2:
    printf("Srop: %d \nTotal Runtime: %d \nAverage wait time: %d\n", drop, totalRun, (sumWait / totalRun));
        break;
    default:
        break;
    }
    
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}



