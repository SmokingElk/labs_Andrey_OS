#include <stdio.h>
#include <stdbool.h>
#include <dlfcn.h>

typedef int (*PrimeCountFunction)(int, int);
typedef int (*GCFFunction)(int, int);

int main () {
    void *libPrimeCountHandle = NULL;
    void *libGCFHandle = NULL;

    PrimeCountFunction primeCountFunction = NULL;
    GCFFunction gcfFunction = NULL;

    char *error;

    while (1) {
        int command, calcWay, a, b, res;
        bool fail = false;

        printf("Input command:\n");
        printf("0 - load libraties\n");
        printf("1 - count primes\n");
        printf("2 - calculate gcf\n");
        printf("-1 - exit\n");

        scanf("%d", &command);

        if (command == -1) break;

        switch (command) {
            case 0:
                if (libPrimeCountHandle) dlclose(libPrimeCountHandle);
                if (libGCFHandle) dlclose(libGCFHandle);

                char primeCountLibName[50], gcfLibName[50];
                printf("Input prime count library name: ");
                scanf("%50s", primeCountLibName);

                libPrimeCountHandle = dlopen(primeCountLibName, RTLD_LAZY);
                if (!libPrimeCountHandle) {
                    fprintf(stderr, "Error loading %s: %s\n", primeCountLibName, dlerror());
                    break;
                }

                printf("Input gcf library name: ");
                scanf("%50s", gcfLibName);

                libGCFHandle = dlopen(gcfLibName, RTLD_LAZY);
                if (!libGCFHandle) {
                    fprintf(stderr, "Error loading %s: %s\n", gcfLibName, dlerror());
                    dlclose(libPrimeCountHandle);
                    break;
                }

                printf("Libraries loaded\n");
                break;

            case 1:
                printf("Input segment borders: ");
                scanf("%d %d", &a, &b);

                printf("Input way to count primes:\n");
                printf("1 - naive\n");
                printf("2 - Eratosthenes' sieve\n");
                scanf("%d", &calcWay);

                switch (calcWay) {
                    case 1:
                        primeCountFunction = (PrimeCountFunction)dlsym(libPrimeCountHandle, "primeCountNaive");
                        break;

                    case 2:
                        primeCountFunction = (PrimeCountFunction)dlsym(libPrimeCountHandle, "primeCountEratosthenes");
                        break;
                    
                    default:
                        printf("Unknown way\n");
                        fail = true;
                }

                if ((error = dlerror()) != NULL) {
                    fprintf(stderr, "Function loading error: %s\n", error);
                    break;
                }

                if (!fail) {
                    res = primeCountFunction(a, b);
                    printf("%d prime numbers\n", res);
                } 

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
                        gcfFunction = (GCFFunction)dlsym(libGCFHandle, "gcfEuclid");
                        break;

                    case 2:
                        gcfFunction = (GCFFunction)dlsym(libGCFHandle, "gcfNaive");
                        break;
                    
                    default:
                        printf("Unknown way\n");
                        fail = true;
                }

                if ((error = dlerror()) != NULL) {
                    fprintf(stderr, "Function loading error: %s\n", error);
                    break;
                }

                if (!fail) {
                    res = gcfFunction(a, b);
                    printf("GCF: %d\n", res);
                } 

                break;
            
            default:
                printf("Unknown command\n");
                break;
        }

        printf("\n");
    }

    if (libPrimeCountHandle) dlclose(libPrimeCountHandle);
    if (libGCFHandle) dlclose(libGCFHandle);
    
    printf("exit\n");

    return 0;
}
