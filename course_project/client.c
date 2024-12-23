#include <stdio.h>
#include "shm_server/shm_server.h"

void responseHandler (Package response) {
    printf("Server response: %s", response->content);
}

int main () {
    Client client = createClient(
        "/server_test", 
        responseHandler
    );

    while (1) {
        int number;
        printf("Input number: ");
        scanf("%d", &number);

        if (number < 0) break;

        char request[100];
        snprintf(request, sizeof(request), "%d", number);

        uuid_t msgId;
        uuid_generate(msgId);
        
        sendMessageClient(client, msgId, request);
    }

    deleteClient(client);
    return 0;
}
