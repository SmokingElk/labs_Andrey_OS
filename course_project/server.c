#include <stdio.h>
#include <stdlib.h>
#include "shm_server/shm_server.h"

void requestHandler (Package request, Server server) {
    int number = atoi(request->content);

    printf("Request: %s -> ", request->content);

    int numberSq = number * number;
    char response[16];
    snprintf(response, sizeof(response), "%d", numberSq);

    printf("Response: %s\n", response);

    sendResponseServer(server, request, response);
}

int main () {
    Server server = createServer(
        "/server_test", 
        requestHandler
    );

    while (1) {
        int command = 0;
        printf("Input 0 to end close\n");
        scanf("%d", &command);

        if (command == 0) break;   
    }

    deleteServer(server);
    return 0;
}
