#include "prime_count.h"
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

int primeCountNaive (int a, int b) {
    int res = 0;

    for (int i = a > 2 ? a : 2; i <= b; i++) {
        int adjust = 1;

        for (int j = 2; j * j <= i; j++) {
            if (i % j == 0) {
                adjust = 0;
                break;
            }
        }

        res += adjust;
    }

    return res;
}

int primeCountEratosthenes (int a, int b) {
    int res = 0;
    bool *sieve = (bool*)malloc(sizeof(bool) * (b + 1));

    for (int i = 0; i <= b; i++) sieve[i] = true;

    for (int i = 2; i <= b; i++) {
        if (!sieve[i]) continue;

        if (i >= a) res++;
        for (int j = 2 * i; j <= b; j += i) sieve[j] = false; 
    }

    free(sieve);

    return res;
}
