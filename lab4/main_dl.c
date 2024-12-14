#include <stdio.h>
#include <stdbool.h>
#include <dlfcn.h>

typedef int (*AreaFunction)(int, int);
typedef int *(*SortFunction)(int*, int, int);

int main () {
    void *libArea = NULL;
    void *libSort = NULL;

    AreaFunction areaFunction = NULL;
    SortFunction sortFunction = NULL;

    char *error;

    while (1) {
        int command, calcWay, a, b, res;
        bool fail = false;

        printf("Input command:\n");
        printf("0 - load libraties\n");
        printf("1 - calc area\n");
        printf("2 - sort array\n");
        printf("-1 - exit\n");

        scanf("%d", &command);

        if (command == -1) break;

        switch (command) {
            case 0:
                if (libArea) dlclose(libArea);
                if (libSort) dlclose(libSort);

                char areaLibName[50], sortLibName[50];
                printf("Input area library name: ");
                scanf("%50s", areaLibName);

                libArea = dlopen(areaLibName, RTLD_LAZY);
                if (!libArea) {
                    fprintf(stderr, "Error loading %s: %s\n", areaLibName, dlerror());
                    break;
                }

                printf("Input sort library name: ");
                scanf("%50s", sortLibName);

                libSort = dlopen(sortLibName, RTLD_LAZY);
                if (!libSort) {
                    fprintf(stderr, "Error loading %s: %s\n", sortLibName, dlerror());
                    dlclose(libArea);
                    break;
                }

                printf("Libraries loaded\n");
                break;

            case 1:
                printf("Input sides: ");
                scanf("%d %d", &a, &b);

                printf("Input figure:\n");
                printf("1 - rectangle\n");
                printf("2 - triangle\n");
                scanf("%d", &calcWay);

                switch (calcWay) {
                    case 1:
                        areaFunction = (AreaFunction)dlsym(libArea, "areaRectangle");
                        break;

                    case 2:
                        areaFunction = (AreaFunction)dlsym(libArea, "areaTriangle");
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
                    res = areaFunction(a, b);
                    printf("area: %d\n", res);
                } 

                break;

            case 2:
                printf("Input numbers\n");

                int arr[10];
                
                for (int i = 0; i < 10; i++) scanf("%d", arr + i);

                printf("Input way to sort:\n");
                printf("1 - bubble\n");
                printf("2 - quick\n");
                scanf("%d", &calcWay);

                switch (calcWay) {
                    case 1:
                        sortFunction = (SortFunction)dlsym(libSort, "sortBubble");
                        break;

                    case 2:
                        sortFunction = (SortFunction)dlsym(libSort, "sortQuick");
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
                    sortFunction(arr, 0, 9);
                    for (int i = 0; i < 10; i++) printf("%d ", arr[i]);
                    printf("\n");
                } 

                break;
            
            default:
                printf("Unknown command\n");
                break;
        }

        printf("\n");
    }

    if (libArea) dlclose(libArea);
    if (libSort) dlclose(libSort);
    
    printf("exit\n");

    return 0;
}
