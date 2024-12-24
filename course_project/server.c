#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shm_server/shm_server.h"
#include "map/map.h"
#include "linkedList/linkedList.h"
#include "config.h"

Server server;
Map games;
Map actions;

typedef struct {
    size_t bulls;
    size_t cows;
    bool full;
} GuessRes;

GuessRes calcGuessResult (char *word, char *assumption) {
    GuessRes res = {0, 0, false};

    for (int i = 0; word[i] != '\0'; i++) {
        if (assumption[i] == '\0') break;
        if (word[i] == assumption[i]) res.bulls++;
    }

    for (int i = 0; assumption[i] != '\0'; i++) {
        for (int j = 0; word[j] != '\0'; j++) {
            if (i == j) continue;
            if (assumption[i] == word[j]) res.cows++;
        }
    }

    res.full = strlen(word) == res.bulls;

    return res;
}

typedef struct {
    char name[PLAYER_NAME_MAX_LEN];
    uuid_t id;
} _Player, *Player;

Player createPlayer (char *name, uuid_t id) {
    Player player = (Player)malloc(sizeof(_Player));

    strncpy(player->name, name, PLAYER_NAME_MAX_LEN);
    uuid_copy(player->id, id);

    return player;
}

void deletePlayer (void *_player) {
    Player player = (Player)_player;
    free(player);
}

typedef struct {
    size_t playersMaxCount;
    size_t playersConnected;
    bool start;
    List players;
    ListIter currentPlayer;
    char word[WORD_MAX_LEN];
    char name[GAME_NAME_MAX_LEN];
} _Game, *Game;

Game createGame (size_t playersMaxCount, char *gameName, char *word) {
    Game game = (Game)malloc(sizeof(_Game));
    
    game->players = createList();
    game->start = false;
    game->playersConnected = 0;
    game->playersMaxCount = playersMaxCount;
    strncpy(game->name, gameName, PLAYER_NAME_MAX_LEN);
    strncpy(game->word, word, WORD_MAX_LEN);

    return game;
}

void deleteGame (void *_game) {
    Game game = (Game)_game;

    for (ListIter it = beginList(game->players); iterNotEquals(it, endList(game->players)); it = iterNext(it)) {
        deletePlayer(iterFetch(Player, it));
    }

    deleteList(game->players);
    free(game);
}

void notifyAll (Game game, char *message) {
    for (ListIter it = beginList(game->players); iterNotEquals(it, endList(game->players)); it = iterNext(it)) {
        Player player = iterFetch(Player, it);
        sendMessageServer(server, player->id, message);
    }
}

bool isCurrentPlayer (Game game, uuid_t id) {
    Player player = iterFetch(Player, game->currentPlayer);
    return uuid_compare(player->id, id) == 0;
}

void nextPlayer (Game game) {
    game->currentPlayer = iterNext(game->currentPlayer);
    if (iterEquals(game->currentPlayer, endList(game->players))) game->currentPlayer = iterNext(game->currentPlayer);

    Player player = iterFetch(Player, game->currentPlayer);

    char message[256];
    snprintf(message, sizeof(message), "%s is moving", player->name);
    notifyAll(game, message);
}

bool addPlayer (Game game, char *name, uuid_t id) {
    if (game->playersConnected >= game->playersMaxCount) return false;

    char message[256];
    snprintf(message, sizeof(message), "player %s joining to game %s", name, game->name);
    notifyAll(game, message);

    Player newPlayer = createPlayer(name, id);
    insertToList(game->players, iterPrev(endList(game->players)), newPlayer);
    game->playersConnected++;

    return true;
}

bool removePlayer (Game game, uuid_t id) {
    for (ListIter it = beginList(game->players); iterNotEquals(it, endList(game->players)); it = iterNext(it)) {
        Player player = iterFetch(Player, it);
        if (uuid_compare(id, player->id) != 0) continue;

        bool needNext = false;
        if (game->start && isCurrentPlayer(game, player->id)) {
            needNext = true;
            game->currentPlayer = iterPrev(game->currentPlayer); 
        }

        char message[256];
        snprintf(message, sizeof(message), "player %s is leaving from game %s", player->name, game->name);

        deletePlayer(player);

        deleteFromList(game->players, it);
        game->playersConnected--;

        notifyAll(game, message);

        if (needNext && game->playersConnected > 0) nextPlayer(game);
        break;
    }
}

void tryStart (Game game) {
    if (game->playersConnected < game->playersMaxCount || (game->start && game->playersConnected != 1)) return;

    game->start = true;
    game->currentPlayer = iterPrev(beginList(game->players));

    char startMessage[256];
    snprintf(startMessage, sizeof(startMessage), "game %s started", game->name);
    notifyAll(game, startMessage);
    
    nextPlayer(game);
}

bool playerAttempt (Game game, uuid_t id, char *assumption, Package request) {
    if (!game->start) {
        sendResponseServer(server, request, "game not started yet");
        return false;
    }

    if (!isCurrentPlayer(game, id)) {
        sendResponseServer(server, request, "not your turn");
        return false;
    }

    GuessRes res = calcGuessResult(game->word, assumption);

    char message[256];
    snprintf(
        message, sizeof(message), "%ld %s and %ld %s", 
        res.cows, res.cows != 1 ? "cows" : "cow", 
        res.bulls, res.bulls != 1 ? "bulls" : "bull"
    );
    sendResponseServer(server, request, message);

    if (!res.full) nextPlayer(game);

    return res.full;
}

void endGame (Game game, uuid_t id) {
    printf("Game end!\n");
    char message[256];

    for (ListIter it = beginList(game->players); iterNotEquals(it, endList(game->players)); it = iterNext(it)) {
        Player player = iterFetch(Player, it);

        if (uuid_compare(player->id, id) == 0) {
            sendMessageServer(server, player->id, "#You are winner!");
            snprintf(message, sizeof(message), "#%s is winner", player->name);
            break;
        }
    }

    for (ListIter it = beginList(game->players); iterNotEquals(it, endList(game->players)); it = iterNext(it)) {
        Player player = iterFetch(Player, it);
        if (uuid_compare(player->id, id) == 0) continue;

        sendMessageServer(server, player->id, message);
    }
}

typedef void (*Action)(Package, Server, char*);

#define sendFail(text) { sendResponseServer(server, request, (text)); return; }

void actionCreate (Package request, Server server, char *content) {
    char *gameName = strtok(NULL, " ");
    if (gameName == NULL) sendFail("game_name not specified");

    char *word = strtok(NULL, " ");
    if (word == NULL) sendFail("word not specified");

    char *playersCountStr = strtok(NULL, " ");
    if (playersCountStr == NULL) sendFail("players_count not specified");
    size_t playersCount = atoi(playersCountStr);

    if (playersCount < 0) sendFail("incorrect players count");

    if (hasMap(games, gameName)) sendFail("game already exists");

    setMap(games, gameName, createGame(playersCount, gameName, word));
    sendResponseServer(server, request, "game is created");
}

void actionJoin (Package request, Server server, char *content) {
    char *gameName = strtok(NULL, " ");
    if (gameName == NULL) sendFail("#game_name not specified");

    char *playerName = strtok(NULL, " ");
    if (playerName == NULL) sendFail("#player_name not specified");

    if (!hasMap(games, gameName)) {
        sendResponseServer(server, request, "#game not exists");
        return;
    }

    Game game = getMap(Game, games, gameName);
    bool success = addPlayer(game, playerName, request->clientID);
    
    if (success) {
        sendResponseServer(server, request, "successfully joined");
        tryStart(game);
    } else sendFail("#max players count reached");
}

void actionLeave (Package request, Server server, char *content) {
    char *gameName = strtok(NULL, " ");
    if (gameName == NULL) sendFail("game_name not specified");

    if (!hasMap(games, gameName)) sendFail("game not exists");

    Game game = getMap(Game, games, gameName);
    removePlayer(game, request->clientID);
    sendResponseServer(server, request, "#leaving this game");
}

void actionAttempt (Package request, Server server, char *content) {
    char *gameName = strtok(NULL, " ");
    if (gameName == NULL) sendFail("game_name not specified");

    char *assumption = strtok(NULL, " ");
    if (gameName == NULL) sendFail("assumption not specified");

    if (!hasMap(games, gameName)) sendFail("game not exists");

    Game game = getMap(Game, games, gameName);

    bool isWin = playerAttempt(game, request->clientID, assumption, request);
    
    if (isWin) {
        endGame(game, request->clientID);
        removeMap(games, game->name);
        deleteGame(game);
    }
}

void requestHandler (Package request, Server server) {
    char content[MAX_CONTENT_LENGTH];
    strncpy(content, request->content, sizeof(content));

    printf("Request: %s\n", content);

    char *requestType = strtok(content, " ");

    if (!hasMap(actions, requestType)) {
        sendResponseServer(server, request, "unknown server action");
        return;
    }

    Action action = getMap(Action, actions, requestType);
    action(request, server, content);
}

int main () {
    games = createMap(10);
    actions = createMap(10);

    setMap(actions, SERVER_ACTION_CREATE, actionCreate);
    setMap(actions, SERVER_ACTION_JOIN, actionJoin);
    setMap(actions, SERVER_ACTION_LEAVE, actionLeave);
    setMap(actions, SERVER_ACTION_ATTEMPT, actionAttempt);
    
    server = createServer(
        SERVER_NAME, 
        requestHandler
    );

    while (1) {
        int command = 0;
        printf("Input 0 to close server\n");
        scanf("%d", &command);

        if (command == 0) break;   
    }

    deleteMap(games, deleteGame);
    deleteServer(server);
    return 0;
}
