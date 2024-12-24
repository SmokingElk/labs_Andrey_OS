#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "shm_server/shm_server.h"
#include "config.h"

char name[PLAYER_NAME_MAX_LEN];
char game[GAME_NAME_MAX_LEN];
bool joined = false;

void responseHandler (Package response) {
    if (response->content[0] == '#') {
        joined = false;
        printf("> %s\n", response->content + 1);
        return;
    }

    printf("> %s\n", response->content);
}

void sendMessage (Client client, char *message) {
    uuid_t id;
    uuid_generate(id);
    
    sendMessageClient(client, id, message);
}

int main () {
    Client client = createClient(
        SERVER_NAME, 
        responseHandler
    );

    printf("Input your name: ");
    fgets(name, sizeof(name), stdin);
    *strchr(name, '\n') = '\0';

    printf("Welcome, %s!\n", name);

    while (1) {
        char userRequest[256];
        fgets(userRequest, sizeof(userRequest), stdin);

        char *userRequestType = strtok(userRequest, " \n");

        if (strcmp(userRequestType, "create") == 0) {
            char *gameName = strtok(NULL, " \n");
            char *word = strtok(NULL, " \n");
            size_t playersCount = atoi(strtok(NULL, " \n"));

            char request[MAX_CONTENT_LENGTH];
            snprintf(request, sizeof(request), "create %s %s %ld ", gameName, word, playersCount);
            sendMessage(client, request);
        } else if (strcmp(userRequestType, "join") == 0) {
            char *gameName = strtok(NULL, " \n");

            if (joined) {
                printf("already joined\n");
                continue;
            }

            joined = true;
            strncpy(game, gameName, sizeof(name));

            char request[MAX_CONTENT_LENGTH];
            snprintf(request, sizeof(request), "join %s %s ", gameName, name);
            sendMessage(client, request);
        } else if (strcmp(userRequestType, "leave") == 0) {
            if (!joined) {
                printf("Not joined\n");
                continue;
            }

            char request[MAX_CONTENT_LENGTH];
            snprintf(request, sizeof(request), "leave %s ", game);
            sendMessage(client, request);
        } else if (strcmp(userRequestType, "attempt") == 0) {
            char *assumption = strtok(NULL, " \n");

            if (!joined) {
                printf("Not joined\n");
                continue;
            }

            char request[MAX_CONTENT_LENGTH];
            snprintf(request, sizeof(request), "attempt %s %s ", game, assumption);
            sendMessage(client, request);
        } else if (strcmp(userRequestType, "exit") == 0) {
            if (joined) {
                char request[MAX_CONTENT_LENGTH];
                snprintf(request, sizeof(request), "leave %s ", game);
                sendMessage(client, request);
                sleep(1);
            }

            break;
        } else {
            printf("Unknown action!\n");
        }
    }

    deleteClient(client);
    return 0;
}
