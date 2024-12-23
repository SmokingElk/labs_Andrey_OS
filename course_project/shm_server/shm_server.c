#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "shm_server.h"

void _initMtx (pthread_mutex_t *mtx) {
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(mtx, &mutex_attr);
}

void _initQueue (_PackageQueue *queue) {
    queue->size = 0;
    queue->topIndex = 0;
}

bool _emptyQueue (_PackageQueue *queue) {
    return queue->size == 0;
}

Package _topQueue (_PackageQueue *queue) {
    return &(queue->location[queue->topIndex]);
}

void _popQueue (_PackageQueue *queue) {
    queue->size--;
    queue->topIndex = (queue->topIndex + 1) % _MAX_MESSAGES_COUNT;
}

bool _pushQueue (_PackageQueue *queue, uuid_t clientID, uuid_t messageID, char *content) {
    if (queue->size >= _MAX_MESSAGES_COUNT) return false;

    size_t tailIndex = (queue->topIndex + queue->size) % _MAX_MESSAGES_COUNT;
    Package newPackage = &queue->location[tailIndex];

    strncpy(newPackage->content, content, _MAX_CONTENT_LENGTH);
    uuid_copy(newPackage->clientID, clientID);
    uuid_copy(newPackage->messageID, messageID);

    queue->size++;
    return true;
}

void _initShmSegment (_ShmLayout *layout) {
    _initMtx(&layout->messageMutex);
    _initMtx(&layout->responseMutex);
    _initQueue(&layout->messageQueue);
    _initQueue(&layout->responseQueue);
}

ShmConnection _createConnection (char *memoryName) {
    ShmConnection connection = (ShmConnection)malloc(sizeof(_ShmConnection));
    
    strncpy(connection->memoryName, memoryName, _SERVER_NAME_MAX_LEN);

    connection->shmFD = shm_open(connection->memoryName, O_RDWR, 0666);
    if (connection->shmFD == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    size_t shmSize = sizeof(_ShmLayout);
    connection->layout = (_ShmLayout*)mmap(NULL, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, connection->shmFD, 0);
    if (connection->layout == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return connection;
}

Package _getResponseConnection (ShmConnection connection, uuid_t clientID) {
    pthread_mutex_lock(&connection->layout->responseMutex);

    if (_emptyQueue(&connection->layout->responseQueue)) {
        pthread_mutex_unlock(&connection->layout->responseMutex);
        return NULL;
    }

    Package response = _topQueue(&connection->layout->responseQueue);

    if (uuid_compare(response->clientID, clientID) != 0) {
        pthread_mutex_unlock(&connection->layout->responseMutex);
        return NULL;
    }

    return response;
}

void _acquireResponseConnection (ShmConnection connection) {
    _popQueue(&connection->layout->responseQueue);
    pthread_mutex_unlock(&connection->layout->responseMutex);
}

bool _sendMessageConnection (ShmConnection connection, uuid_t clientID, uuid_t messageID, char *content) {
    pthread_mutex_lock(&connection->layout->messageMutex);
    bool sended = _pushQueue(&connection->layout->messageQueue, clientID, messageID, content);
    pthread_mutex_unlock(&connection->layout->messageMutex);
    return sended;
}

void _closeConnection (ShmConnection connection) {
    munmap(connection->layout, sizeof(_ShmLayout));
    free(connection);
}

HostConnection _createHostConnection (char *memoryName) {
    HostConnection connection = (HostConnection)malloc(sizeof(_ShmConnection));

    strncpy(connection->memoryName, memoryName, _SERVER_NAME_MAX_LEN);

    connection->shmFD = shm_open(connection->memoryName, O_CREAT | O_RDWR, 0666);
    if (connection->shmFD == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    size_t size = sizeof(_ShmLayout);

    if (ftruncate(connection->shmFD, size) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    connection->layout = (_ShmLayout*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, connection->shmFD, 0);
    if (connection->layout == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    _initShmSegment(connection->layout);

    return connection;
}

Package _getMessageHostConnection (HostConnection connection) {
    pthread_mutex_lock(&connection->layout->messageMutex);

    if (_emptyQueue(&connection->layout->messageQueue)) {
        pthread_mutex_unlock(&connection->layout->messageMutex);
        return NULL;
    }

    return _topQueue(&connection->layout->messageQueue);
}

void _acquireMessageHostConnection (HostConnection connection) {
    _popQueue(&connection->layout->messageQueue);
    pthread_mutex_unlock(&connection->layout->messageMutex);
}

bool _sendResponseHostConnection (HostConnection connection, uuid_t clientID, uuid_t messageID, char *content) {
    pthread_mutex_lock(&connection->layout->responseMutex);
    bool sended = _pushQueue(&connection->layout->responseQueue, clientID, messageID, content);
    pthread_mutex_unlock(&connection->layout->responseMutex);
    return sended;
}

void _closeHostConnection (HostConnection connection) {
    munmap(connection->layout, sizeof(_ShmLayout));
    shm_unlink(connection->memoryName);
    free(connection);
}

void *_responseListenerFunc (void *_client) {
    Client client = (Client)_client;

    while (client->working) {
        Package response = _getResponseConnection(client->connection, client->id);
        if (response == NULL) continue;

        client->responseHandler(response);
        puts("");
        _acquireResponseConnection(client->connection);
    }

    return 0;
}

Client createClient (char *serverName, ResponseHandler responseHandler) {
    Client client = (Client)malloc(sizeof(_Client));

    client->working = true;
    
    client->connection = (ShmConnection)_createConnection(serverName);
    uuid_generate(client->id);

    client->responseHandler = responseHandler;
    pthread_create(&client->responseListener, NULL, _responseListenerFunc, client);

    return client;
}

void sendMessageClient (Client client, uuid_t msgId, char *content) {
    _sendMessageConnection(client->connection, client->id, msgId, content);
}

void deleteClient (Client client) {
    client->working = false;

    pthread_join(client->responseListener, NULL);

    _closeConnection(client->connection);
    free(client);
}

void *_requestListenerFunc (void *_server) {
    Server server = (Server)_server;

    while (server->working) {
        Package message = _getMessageHostConnection(server->connection);
        if (message == NULL) continue;

        server->requestHandler(message, server);
        _acquireMessageHostConnection(server->connection);
    }

    return 0;
}

Server createServer (char *serverName, RequestHandler requestHandler) {
    Server server = (Server)malloc(sizeof(_Server));

    server->working = true;
    
    server->connection = (HostConnection)_createHostConnection(serverName);
    
    server->requestHandler = requestHandler;

    pthread_create(&server->requestListener, NULL, _requestListenerFunc, server);

    return server;
}

void sendResponseServer (Server server, Package responseTo, char *content) {
    _sendResponseHostConnection(server->connection, responseTo->clientID, responseTo->messageID, content);
}

void deleteServer (Server server) {
    server->working = false;

    pthread_join(server->requestListener, NULL);

    _closeHostConnection(server->connection);
    free(server);
}

