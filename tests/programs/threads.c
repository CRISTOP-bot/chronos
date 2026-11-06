#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void* worker(void* arg) {
    printf("worker %d\\n", *(int*)arg);
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    int a=1, b=2;
    pthread_create(&t1, NULL, worker, &a);
    pthread_create(&t2, NULL, worker, &b);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("done\\n");
    return 0;
}
