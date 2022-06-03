#include "helpers.h"

void usage(char *file) {
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int cli_sock, ret;
    int nagle_flag = 1;
    struct sockaddr_in serv_adr;
    char recv_buff[MAXRECV];
    char buffer[BUFLEN];
    fd_set read_set, temp_set, except_set;
    tcp_msg *msg_from_server;
    msg_to_server msg_to_send;
    FD_ZERO(&read_set);
    FD_ZERO(&temp_set);
    FD_ZERO(&except_set);
    if (argc < 4) {
        usage(argv[0]);
    }

    cli_sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(cli_sock < 0, "Socket error");

    serv_adr.sin_family = AF_INET;
    serv_adr.sin_port = htons(atoi(argv[3]));
    ret = connect(cli_sock, (struct sockaddr *) &serv_adr, sizeof(serv_adr));
    DIE(ret < 0, "Connection failed");
    FD_SET(cli_sock, &read_set);
    FD_SET(0, &read_set);
    ret = send(cli_sock, argv[1], strlen(argv[1]), 0);
    DIE(ret < 0, "Send failed");
    setsockopt(cli_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&nagle_flag, sizeof(int));
    while(1) {
        temp_set = read_set;
        ret = select(cli_sock + 1, &temp_set, NULL, NULL, NULL);
        DIE(ret < 0, "Select failed");
        if (FD_ISSET(0, &temp_set)) {
            memset(buffer, 0, BUFLEN);
            ret = read(0, buffer, sizeof(buffer) - 1);
            DIE(ret < 0, "Read failed");
            if (strncmp(buffer, "exit", 4) == 0) {
                break;
            }
            // Vedem daca clientul vrea sa dea subscribe sau unsubscribe
            char *copy = strdup(buffer);
            char *token = strtok(copy, " ");
            int subscribed = 1;
            if (strncmp(token, "subscribe", 9) == 0) {
                strcpy(msg_to_send.command, "subscribe");
                // Extragem topic
                token = strtok(NULL, " ");
                if (token != NULL) {
                    strcpy(msg_to_send.topic, token);
                } else {
                    subscribed = -1;
                    fprintf(stderr, "Please introduce topic\n");
                }
                token = strtok(NULL, " ");
                if (token != NULL) {
                    int sf = atoi(token);
                    if (sf != 0 && sf != 1) {
                        subscribed = -1;
                        fprintf(stderr, "Invalid SF\n");
                    } else {
                        msg_to_send.sf = sf;
                    }
                } else {
                    subscribed = -1;
                    fprintf(stderr, "Please introduce SF parameter\n");
                }
            } else if (strncmp(token, "unsubscribe", 11) == 0) {
                subscribed = 0;
                strcpy(msg_to_send.command, "unsubscribe");
                // Extragem topic
                token = strtok(NULL, " ");
                if (token != NULL) {
                    token[strlen(token) - 1] = '\0';
                    strcpy(msg_to_send.topic, token);
                }
                msg_to_send.sf = 0;
            } else {
                fprintf(stderr, "Invalid command\n");
                subscribed = -1;
            }
            ret = send(cli_sock, (char *) &msg_to_send, sizeof(msg_to_send), 0);
            DIE(ret < 0, "Send failed");
            if (subscribed == 1) {
                printf("Subscribed to topic.\n");
            } else if (subscribed == 0) {
                printf("Unsubscribed from topic.\n");
            }
        }
        if (FD_ISSET(cli_sock, &temp_set)) {
            memset(recv_buff, 0, MAXRECV);
            ret = recv(cli_sock, recv_buff, sizeof(recv_buff), 0);
            DIE(ret < 0, "Receive failed");
            if (ret == 0) {
                break;
            }
            
            if (ret < MAXRECV) {
                int stop = MAXRECV - ret;
                int adding = ret;
                while (stop > 0) {
                    ret = recv(cli_sock, recv_buff + adding, stop, 0);
                    if (ret < 0) {
                        DIE(ret < 0, "Receive failed");
                        break;
                    }
                    adding += ret;
                    stop -= ret;
                }
            }
            msg_from_server = (tcp_msg *) recv_buff;
            printf("%s:%hu - %s - %s - %s\n", msg_from_server->ip_client_udp,
            msg_from_server->port_client_udp, msg_from_server->topic,
            msg_from_server->type, msg_from_server->msg);
        }
    }
    close(cli_sock);
    return 0;
}
