#include <stdio.h>
#include <stdbool.h>
#include "area.h"
#include "sort.h"

int main () {
    while (1) {
        int command, calcWay, a, b, res;
        bool fail = false;

        printf("Input command:\n");
        printf("1 - calc area\n");
        printf("2 - sort array\n");
        printf("-1 - exit\n");

        scanf("%d", &command);

        if (command == -1) break;

        switch (command) {
            case 1:
                printf("Input sides: ");
                scanf("%d %d", &a, &b);

                printf("Input figure:\n");
                printf("1 - rectangle\n");
                printf("2 - triangle\n");
                scanf("%d", &calcWay);

                switch (calcWay) {
                    case 1:
                        res = areaRectangle(a, b);
                        break;

                    case 2:
                        res = areaTriangle(a, b);
                        break;
                    
                    default:
                        printf("Unknown way\n");
                        fail = true;
                }

                if (!fail) printf("Area: %d\n", res);

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
                        sortBubble(arr, 0, 9);
                        break;

                    case 2:
                        sortQuick(arr, 0, 9);
                        break;
                    
                    default:
                        printf("Unknown way\n");
                        fail = true;
                }

                if (!fail) for (int i = 0; i < 10; i++) printf("%d ", arr[i]);
                printf("\n");

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
