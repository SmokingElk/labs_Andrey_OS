#include "sort.h"

int *sortBubble (int *arr, int low, int high) {
    int i, j;

    for (i = 0; i < 10 - 1; i++) {
        for (j = 0; j < 10 - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }

    return arr;
}

int *sortQuick (int *arr, int low, int high) {
    if (low < high) {
        int pivot = arr[high]; 
        int i = low - 1;      

        for (int j = low; j < high; j++) {
            if (arr[j] <= pivot) {
                i++;
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }

        int temp = arr[i + 1];
        arr[i + 1] = arr[high];
        arr[high] = temp;

        int pi = i + 1;

        sortQuick(arr, low, pi - 1);
        sortQuick(arr, pi + 1, high);
    }
    return arr;
}