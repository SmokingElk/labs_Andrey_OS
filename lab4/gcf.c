#include "prime_count.h"

int gcfEuclid (int a, int b) {
    while (a != 0 && b != 0) {
        if (a > b) a = a % b;
        else b = b % a;        
    }

    return a + b;
}

int gcfNaive (int a, int b) {
    int gcf = 1;

    for (int i = 2; i < b && i < a; i++) {
        if (a % i == 0 && b % i == 0) gcf = i;
    }

    return gcf;
}
