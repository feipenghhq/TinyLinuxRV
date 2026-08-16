//#include <stdio.h>

int fib(int n) {
    if (n == 0) return 0;
    if (n == 1) return 1;
    return fib(n - 1) + fib(n - 2);
}

int main(void) {
    int n;
    n = fib(10);
    //printf("fib 10 = %d\n", n);

    if (n == 55 ) {
        return 0;
    }
    return 1;
}
