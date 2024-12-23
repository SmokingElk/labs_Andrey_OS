#pragma once
#include <uuid/uuid.h>
#include <stdbool.h>
#include "pthread.h"

#define _SERVER_NAME_MAX_LEN 64
#define _MAX_CONTENT_LENGTH 256
#define _MAX_MESSAGES_COUNT 64

typedef char _Content[_MAX_CONTENT_LENGTH];

typedef struct {
    _Content content;
    uuid_t clientID;
    uuid_t messageID;
} _Package, *Package;

typedef struct {
    _Package location[_MAX_MESSAGES_COUNT];
    size_t size;
    size_t topIndex;
} _PackageQueue;

typedef struct {
    pthread_mutex_t messageMutex;
    _PackageQueue messageQueue;

    pthread_mutex_t responseMutex;
    _PackageQueue responseQueue;
} _ShmLayout;

typedef struct {
    int shmFD; 
    _ShmLayout *layout;
    char memoryName[_SERVER_NAME_MAX_LEN];
} _ShmConnection, *ShmConnection;

typedef ShmConnection HostConnection;

typedef void (*ResponseHandler)(Package);

typedef struct {
    ShmConnection connection;
    uuid_t id;
    pthread_t responseListener;
    ResponseHandler responseHandler;
    bool working;
} _Client, *Client;

Client createClient (char *serverName, ResponseHandler responseHandler);
void sendMessageClient (Client client, uuid_t msgId, char *content);
void deleteClient (Client client);

typedef struct Server {
    HostConnection connection;
    pthread_t requestListener;
    void (*requestHandler)(Package, struct Server*);
    bool working;
} _Server, *Server;

typedef void (*RequestHandler)(Package, Server);

Server createServer (char *serverName, RequestHandler requestHandler);
void sendResponseServer (Server server, Package responseTo, char *content);
void deleteServer (Server server);
