#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>

int nodeId = -1;
bool running = false;

const char *RABBIT_HOST = "localhost";
int RABBIT_PORT = 5672;
amqp_connection_state_t connection;
amqp_channel_t channel = 1;

void die_amqp_rpc_reply(const char *context, amqp_rpc_reply_t r) {
    switch (r.reply_type) {
        case AMQP_RESPONSE_NORMAL:
            return;

        case AMQP_RESPONSE_NONE:
            fprintf(stderr, "%s: missing RPC reply type!\n", context);
            break;

        case AMQP_RESPONSE_LIBRARY_EXCEPTION:
            fprintf(stderr, "%s: %s\n", context, amqp_error_string2(r.library_error));
            break;

        case AMQP_RESPONSE_SERVER_EXCEPTION:
            if (r.reply.id == AMQP_CONNECTION_CLOSE_METHOD) {
                amqp_connection_close_t *m = (amqp_connection_close_t *) r.reply.decoded;
                fprintf(stderr, "%s: server connection error %uh, message: %.*s\n",
                        context, m->reply_code, (int)m->reply_text.len, (char *)m->reply_text.bytes);
            } else if (r.reply.id == AMQP_CHANNEL_CLOSE_METHOD) {
                amqp_channel_close_t *m = (amqp_channel_close_t *) r.reply.decoded;
                fprintf(stderr, "%s: server channel error %uh, message: %.*s\n",
                        context, m->reply_code, (int)m->reply_text.len, (char *)m->reply_text.bytes);
            } else {
                fprintf(stderr, "%s: unknown server error, method id 0x%08X\n", context, r.reply.id);
            }
            break;
    }
    exit(1);
}

void exitWithError(const char *message) {
    fputs(message, stderr);
    exit(EXIT_FAILURE);
}

void initRabbitMQ() {
    connection = amqp_new_connection();
    amqp_socket_t *socket = amqp_tcp_socket_new(connection);
    if (!socket) exitWithError("Creating TCP socket failed\n");
    if (amqp_socket_open(socket, RABBIT_HOST, RABBIT_PORT)) exitWithError("Opening TCP socket failed\n");

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

    die_amqp_rpc_reply("Login", r);

    amqp_channel_open(connection, channel);
    r = amqp_get_rpc_reply(connection);
    die_amqp_rpc_reply("Opening channel", r);

    char queue_name[64];
    snprintf(queue_name, sizeof(queue_name), "slave_queue_%d", nodeId);

    amqp_queue_declare(connection, channel, amqp_cstring_bytes(queue_name), 0, 0, 0, 0, amqp_empty_table);
    r = amqp_get_rpc_reply(connection);
    if (r.reply_type != AMQP_RESPONSE_NORMAL) exitWithError("Queue declare failed");

    amqp_basic_consume(connection, channel, amqp_cstring_bytes(queue_name), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    r = amqp_get_rpc_reply(connection);
    if (r.reply_type != AMQP_RESPONSE_NORMAL) exitWithError("Consume failed");

    amqp_queue_purge(connection, channel, amqp_cstring_bytes(queue_name));
}

void sendResponse(const char *correlation_id, const char *body) {
    amqp_basic_properties_t props;
    memset(&props, 0, sizeof(props));
    props._flags = AMQP_BASIC_CORRELATION_ID_FLAG;
    props.correlation_id = amqp_cstring_bytes(correlation_id);

    amqp_basic_publish(
        connection, 
        channel, 
        amqp_empty_bytes, 
        amqp_cstring_bytes("master_queue"),
        0, 
        0, 
        &props, 
        amqp_cstring_bytes(body)
    );
}

int main(int argc, char const *argv[]) {
    nodeId = atoi(argv[1]);
    
    initRabbitMQ();

    while (1) {
        amqp_envelope_t envelope;
        amqp_maybe_release_buffers(connection);
        amqp_rpc_reply_t res = amqp_consume_message(connection, &envelope, NULL, 0);

        if (res.reply_type == AMQP_RESPONSE_NORMAL) {
            if (envelope.message.properties._flags & AMQP_BASIC_CORRELATION_ID_FLAG) {
                char corr_id[256];
                snprintf(corr_id, sizeof(corr_id), "%.*s",
                         (int)envelope.message.properties.correlation_id.len,
                         (char *)envelope.message.properties.correlation_id.bytes);

                char cmd[256];
                strncpy(cmd, (char *)envelope.message.body.bytes, (int)envelope.message.body.len);

                amqp_basic_ack(connection, channel, envelope.delivery_tag, 0);

                int n = atoi(strtok(cmd, " \n"));

                int sum = 0;
                for (int i = 0; i < n; i++) sum += atoi(strtok(NULL, " \n"));

                char resp[100];
                sprintf(resp, "slave ok:%d: %d", nodeId, sum);
                sendResponse(corr_id, resp);
            }

            amqp_destroy_envelope(&envelope);
        } else {
            fprintf(stderr, "Error: amqp_consume_message failed\n");
            break;
        }
    }

    amqp_channel_close(connection, channel, AMQP_REPLY_SUCCESS);
    amqp_connection_close(connection, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(connection);
    
    return 0;
}
