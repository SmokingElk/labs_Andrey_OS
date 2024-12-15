#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <uuid/uuid.h>
#include <signal.h>
#include <sys/wait.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>

#define OPERATION_CREATE_PROCESS "create"
#define OPERATION_EXEC "exec"
#define OPERATION_EXIT "exit"
#define OPERATION_PING "pingall"
#define OPERATION_PRINT "show"

const char *RABBIT_HOST = "localhost";
int RABBIT_PORT = 5672;
amqp_connection_state_t connection;
amqp_channel_t channel = 1;

typedef struct Node {
    int id;
    pid_t pid;
    bool available;
    struct Node *left;
    struct Node *right;
} _Topology, *Topology;

Topology topology = NULL;

Topology createTopology () {
    return NULL;
}

void deleteTopology (Topology topology, void (*handler)(Topology)) {
    if (topology == NULL) return;
    deleteTopology(topology->left, handler);
    deleteTopology(topology->right, handler);
    handler(topology);
    free(topology);
}

bool addNodeTopology (Topology *topology, int id, pid_t pid) {
    if (*topology != NULL) {
        if (id < (*topology)->id) return addNodeTopology(&((*topology)->left), id, pid);
        if (id > (*topology)->id) return addNodeTopology(&((*topology)->right), id, pid);
        return false;
    } 

    Topology newNode = (Topology)malloc(sizeof(_Topology));

    newNode->left = NULL;
    newNode->right = NULL;
    newNode->id = id;
    newNode->pid = pid;
    newNode->available = true;

    *topology = newNode;
    return true;
}   

bool checkIfAvailable (Topology topology, int id) {
    if (topology == NULL) return false;
    if (topology->id == id) return topology->available;
    return checkIfAvailable(topology->left, id) || checkIfAvailable(topology->right, id);
}

void setNotAvailable (Topology topology, pid_t pid) {
    if (topology == NULL) return;
    if (topology->pid == pid) {
        topology->available = false;
        return;
    }

    setNotAvailable(topology->left, pid);
    setNotAvailable(topology->right, pid);
}

pid_t getPidTopology (Topology topology, int id) {
    if (topology == NULL) return -1;
    if (topology->id == id) return topology->pid;
    if (id < topology->id) return getPidTopology(topology->left, id);
    return getPidTopology(topology->right, id);
}

void deleteHandler (Topology node) {
    if (kill(node->pid, SIGKILL) == -1) {
        perror("kill");
        exit(EXIT_FAILURE);
    }
}

bool printNotAvailable (Topology topology) {
    if (topology == NULL) return false;
    if (!topology->available) printf("%d ", topology->id);

    bool left = printNotAvailable(topology->left);
    bool right = printNotAvailable(topology->right);
    return left || right || !topology->available;
}

void printTopology(Topology topology, int offset) {
    if (topology == NULL) return;
    printTopology(topology->right, offset + 1);
    for (int i = 0; i < offset; i++) printf("\t");
    printf("Slave %d (pid: %d) - %s\n", topology->id, (int)topology->pid, topology->available ? "available" : "not available");
    printTopology(topology->left, offset + 1);
}

pid_t buySlave (Topology *topology, int id) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        char idArg[16];
        sprintf(idArg, "%d", id);
        if (execl("./slave_node.out", "./slave_node.out", idArg, NULL) == -1) perror("execv error");
        return -1;
    } 

    addNodeTopology(topology, id, pid);
    return pid;
}

void exitWithError(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void initRabbitMQ() {
    connection = amqp_new_connection();
    amqp_socket_t *socket = amqp_tcp_socket_new(connection);
    if (!socket) exitWithError("Creating TCP socket failed");

    if (amqp_socket_open(socket, RABBIT_HOST, RABBIT_PORT)) exitWithError("Opening TCP socket failed");

    amqp_rpc_reply_t r = amqp_login(
        connection, 
        "/", 
        0, 
        131072, 
        0,
        AMQP_SASL_METHOD_PLAIN, 
        "guest", 
        "guest"
    );
    
    if (r.reply_type != AMQP_RESPONSE_NORMAL) exitWithError("Login failed");

    amqp_channel_open(connection, channel);
    r = amqp_get_rpc_reply(connection);
    if (r.reply_type != AMQP_RESPONSE_NORMAL) exitWithError("Opening channel failed");

    amqp_queue_declare(connection, channel, amqp_cstring_bytes("master_queue"), 0, 0, 0, 0, amqp_empty_table);
    r = amqp_get_rpc_reply(connection);
    if (r.reply_type != AMQP_RESPONSE_NORMAL) exitWithError("Queue declare failed");

    amqp_basic_consume(connection, channel, amqp_cstring_bytes("master_queue"), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    r = amqp_get_rpc_reply(connection);
    if (r.reply_type != AMQP_RESPONSE_NORMAL) exitWithError("Consume on master_queue failed");

    amqp_queue_purge(connection, channel, amqp_cstring_bytes("master_queue"));
}

void sendMessage (int nodeId, char *cmd) {
    char queue_name[64];
    snprintf(queue_name, sizeof(queue_name), "slave_queue_%d", nodeId);

    uuid_t uuid;
    uuid_generate(uuid);
    char corr_id[37];
    uuid_unparse_lower(uuid, corr_id);

    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CORRELATION_ID_FLAG;
    props.correlation_id = amqp_cstring_bytes(corr_id);

    amqp_basic_publish(
        connection, 
        channel, 
        amqp_empty_bytes, 
        amqp_cstring_bytes(queue_name),
        0, 
        0, 
        &props, 
        amqp_cstring_bytes(cmd)
    );
}

void *messageFromSlavesHandler(void *arg) {
    while (1) {
        amqp_envelope_t envelope;
        amqp_maybe_release_buffers(connection);
        amqp_rpc_reply_t res = amqp_consume_message(connection, &envelope, NULL, 0);
        amqp_basic_ack(connection, channel, envelope.delivery_tag, 0);
        if (res.reply_type == AMQP_RESPONSE_NORMAL) {
            printf("%.*s\n$ ", (int)envelope.message.body.len, (char *)envelope.message.body.bytes);
            fflush(stdout);

            amqp_destroy_envelope(&envelope);
        } else {
            printf("Error: amqp_consume_message failed: %s\n", amqp_error_string2(res.reply_type));
            break;
        }
    }
    return NULL;
}

void dieHandler(int signo) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        setNotAvailable(topology, pid);
    }
}

int main () {
    initRabbitMQ();

    topology = createTopology();

    pthread_t thr;
    pthread_create(&thr, NULL, messageFromSlavesHandler, NULL);
    pthread_detach(thr);

    struct sigaction action;
    action.sa_handler = dieHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &action, NULL);

    bool working = true;

    while (working) {
        char command[128];

        printf("$ ");
        fgets(command, sizeof(command), stdin);

        char *operation = strtok(command, " \n");

        if (strcmp(operation, OPERATION_CREATE_PROCESS) == 0) {
            int id = atoi(strtok(NULL, " \n"));
            if (getPidTopology(topology, id) != -1) {
                printf("Error: Already Exists\n");
                continue;
            }

            pid_t pidCreated = buySlave(&topology, id);
            printf("Ok: %d\n", (int)pidCreated); 
        } else if (strcmp(operation, OPERATION_EXEC) == 0) {
            int id = atoi(strtok(NULL, " \n"));
            if (getPidTopology(topology, id) == -1) {
                printf("Error: %d not found\n", id);
                continue;
            }

            if (!checkIfAvailable(topology, id)) {
                printf("Error: %d not available\n", id);
                continue;
            }

            int n = atoi(strtok(NULL, " \n"));
            int arr[100];

            for (int i = 0; i < n; i++) {
                arr[i] = atoi(strtok(NULL, " \n"));
            }

            char request[256];
            sprintf(request, "%d ", n);

            for (int i = 0; i < n; i++) {
                sprintf(request + strlen(request), "%d ", arr[i]);
            }

            sendMessage(id, request);
        } else if (strcmp(operation, OPERATION_PING) == 0) {
            printf("Ok: ");
            if (!printNotAvailable(topology)) printf("-1");
            printf("\n");
        } else if (strcmp(operation, OPERATION_PRINT) == 0) {
            printTopology(topology, 0);
        } else if (strcmp(operation, OPERATION_EXIT) == 0) {
            break;
        } else {
            printf("Unknown command\n");
        }
    }

    deleteTopology(topology, deleteHandler);
    return 0;
}
