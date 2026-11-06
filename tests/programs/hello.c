#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("hello chronos\n");
    int *p = malloc(16);
    p[0] = 42;
    printf("value=%d\n", p[0]);
    free(p);
    return 0;
}
