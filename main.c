#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define THREAD_CNT 3
int pthread_create(
    pthread_t *thread,
    const pthread_attr_t *attr,
    void *(*start_routine) (void *),
    void *arg
);

void pthread_exit(void *value_ptr);

pthread_t pthread_self(void);

// waste some time
void *func(void *arg) {
    printf("thread %lu: arg = %lu\n", (unsigned long)pthread_self(), (unsigned long)arg);
    unsigned long i;
    for(i = 0; i < 2; i++) {
        printf("thread %lu: print\n", (unsigned long)pthread_self());
        pause();
    }
    return 0;
}

int main(int argc, char **argv) {
    pthread_t threads[THREAD_CNT];
    unsigned long i;

    //create THREAD_CNT threads
    for(i = 0; i < THREAD_CNT; i++) {
    pthread_create(&threads[i], NULL, func, (void *)(i + 100));
    printf("thread %lu created\n", (unsigned long)(threads[i]));
}

for(i = 0; i < 4; i++) {
    printf("thread main: print\n");
    pause();
}

    return 0;
}