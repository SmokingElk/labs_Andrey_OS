#include <stdio.h>
#include <stdbool.h>
#include "prime_count.h"
#include "gcf.h"

int main () {
    while (1) {
        int command, calcWay, a, b, res;
        bool fail = false;

        printf("Input command:\n");
        printf("1 - count primes\n");
        printf("2 - calculate gcf\n");
        printf("-1 - exit\n");

        scanf("%d", &command);

        if (command == -1) break;

        switch (command) {
            case 1:
                printf("Input segment borders: ");
                scanf("%d %d", &a, &b);

                printf("Input way to count primes:\n");
                printf("1 - naive\n");
                printf("2 - Eratosthenes' sieve\n");
                scanf("%d", &calcWay);

                switch (calcWay) {
                    case 1:
                        res = primeCountNaive(a, b);
                        break;

                    case 2:
                        res = primeCountEratosthenes(a, b);
                        break;
                    
                    default:
                        printf("Unknown way\n");
                        fail = true;
                }

                if (!fail) printf("%d prime numbers\n", res);

                break;

            case 2:
                printf("Input A and B: ");
                scanf("%d %d", &a, &b);

                printf("Input way to find gcf:\n");
                printf("1 - Euclidean\n");
                printf("2 - naive\n");
                scanf("%d", &calcWay);

                switch (calcWay) {
                    case 1:
                        res = gcfEuclid(a, b);
                        break;

                    case 2:
                        res = gcfNaive(a, b);
                        break;
                    
                    default:
                        printf("Unknown way\n");
                        fail = true;
                }

                if (!fail) printf("GCF: %d\n", res);

                break;
            
            default:
                printf("Unknown command\n");
                break;
        }

        printf("\n");
    }
    
    printf("exit\n");

    return 0;
}
