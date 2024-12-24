#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "map/map.h"
#include "linkedList/linkedList.h"
#include "shm_server/shm_server.h"
#include "config.h"

#define CLIENT_ACTION_CREATE "create"
#define CLIENT_ACTION_LEAVE "leave"
#define CLIENT_ACTION_JOIN "join"
#define CLIENT_ACTION_ATTEMPT "attempt"
#define CLIENT_ACTION_EXIT "exit"

char name[PLAYER_NAME_MAX_LEN];
char game[GAME_NAME_MAX_LEN];
bool joined = false;

Map actions;

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

bool printHint (char *hint) {
    printf("Hint: %s\n", hint);
    return false;
}

typedef bool (*Action)(Client, char*);

bool actionCreate (Client client, char *userRequest) {
    char *gameName = strtok(NULL, " \n");
    if (gameName == NULL) return printHint("create $game_name $word $players_count");

    char *word = strtok(NULL, " \n");
    if (word == NULL) return printHint("create $game_name $word $players_count");

    char *playersCountStr = strtok(NULL, " \n");
    if (playersCountStr == NULL) return printHint("create $game_name $word $players_count");
    size_t playersCount = atoi(playersCountStr);

    if (playersCount <= 0) return printHint("players count must be positive");

    char request[MAX_CONTENT_LENGTH];
    snprintf(request, sizeof(request), "%s %s %s %ld ", SERVER_ACTION_CREATE, gameName, word, playersCount);
    sendMessage(client, request);

    return false;    
}

bool actionJoin (Client client, char *userRequest) {
    if (joined) return printHint("already joined");
    
    char *gameName = strtok(NULL, " \n");
    if (gameName == NULL) return printHint("join $game_name");

    joined = true;
    strncpy(game, gameName, sizeof(name));

    char request[MAX_CONTENT_LENGTH];
    snprintf(request, sizeof(request), "%s %s %s ", SERVER_ACTION_JOIN, gameName, name);
    sendMessage(client, request);

    return false;    
}

bool actionLeave (Client client, char *userRequest) {
    if (!joined) return printHint("not joined");

    char request[MAX_CONTENT_LENGTH];
    snprintf(request, sizeof(request), "%s %s ", SERVER_ACTION_LEAVE, game);
    sendMessage(client, request);

    return false;    
}

bool actionAttempt (Client client, char *userRequest) {
    if (!joined) return printHint("not joined");

    char *assumption = strtok(NULL, " \n");
    if (assumption == NULL) return printHint("attempted $assumption");

    char request[MAX_CONTENT_LENGTH];
    snprintf(request, sizeof(request), "%s %s %s ", SERVER_ACTION_ATTEMPT, game, assumption);
    sendMessage(client, request);

    return false;    
}

bool actionExit (Client client, char *userRequest) {
    if (joined) {
        char request[MAX_CONTENT_LENGTH];
        snprintf(request, sizeof(request), "%s %s ", SERVER_ACTION_LEAVE, game);
        sendMessage(client, request);
        sleep(1);
    }

    return true;    
}

int main () {
    actions = createMap(10);

    setMap(actions, CLIENT_ACTION_CREATE, actionCreate);
    setMap(actions, CLIENT_ACTION_JOIN, actionJoin);
    setMap(actions, CLIENT_ACTION_LEAVE, actionLeave);
    setMap(actions, CLIENT_ACTION_ATTEMPT, actionAttempt);
    setMap(actions, CLIENT_ACTION_EXIT, actionExit);

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

        if (!hasMap(actions, userRequestType)) {
            printf("Unknown action!\n");
            continue;
        }

        Action action = getMap(Action, actions, userRequestType);
        if (action(client, userRequest)) break;
    }

    deleteClient(client);
    return 0;
}
