#include "helpers.h"

void usage_server(char *file) {
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

int search_topic(vector<topic_struct> topics, char searched[51]) {
    int i;
    for(i = 0; i < topics.size(); i++) {
        if (strcmp(topics[i].topic, searched) == 0) {
            return i;
        }
    }
    return -1;
}

int topic_position(vector<topic_struct> topics, char searched[51]) {
    int i;
    for(i = 0; i < topics.size(); i++) {
        if (strcmp(topics[i].topic, searched) == 0) {
            return i;
        }
    }
    return -1;
}

void convert_to_tcp(udp_msg *msg_udp, tcp_msg *msg_tcp) {
    memset(msg_tcp->topic, 0, sizeof(msg_tcp->topic));
    memset(msg_tcp, 0, sizeof(msg_tcp));
    strcpy(msg_tcp->topic, msg_udp->topic);
    int int_sign;
    double double_sign = 0;
    if (msg_udp->type == 0) {
        strcpy(msg_tcp->type, "INT");
        if (msg_udp->msg[0]) {
            int_sign = -1;
        } else {
            int_sign = 1;
        }
        sprintf(msg_tcp->msg, "%d", int_sign * (ntohl(*(uint32_t *)(msg_udp->msg + 1))));
    } else if(msg_udp->type == 1) {
        strcpy(msg_tcp->type, "SHORT_REAL");
        double_sign = 1.0 * (ntohs(*(uint16_t *)(msg_udp->msg))) / 100;
        sprintf(msg_tcp->msg, "%.2f", double_sign);
    } else if (msg_udp->type == 2) {
        strcpy(msg_tcp->type, "FLOAT");
        if (msg_udp->msg[0]) {
            double_sign = -1.0;
        } else {
            double_sign = 1.0;
        }
        double_sign *= (ntohl(*(uint32_t *)(msg_udp->msg + 1)));
        double_sign = double_sign / pow(10.0, msg_udp->msg[5]);
        sprintf(msg_tcp->msg, "%lf", double_sign);
    } else if (msg_udp->type == 3) {
        strcpy(msg_tcp->type, "STRING");
        strcpy(msg_tcp->msg, msg_udp->msg);
    } else {
        fprintf(stderr, "Invalid data_type\n");
    }
}

int found_topic(vector<topic_struct> topics, char searched[51]) {
    int i;
    for (i = 0; i < topics.size(); i++) {
        if (strcmp(topics[i].topic, searched) == 0) {
            return 1;
        }
    }
    return 0;
}

int send_msg(int socket, char *msg) {
    int stop = MAXRECV;
    while (stop > 0) {
        int res = send(socket, msg, stop, 0);
        if (res <= 0) {
            return 0;
        }
        msg += res;
        stop -= res;
    }
    return 1;
}

int client_sf(vector<topic_struct> topics, char searched[51]) {
    int i;
    for (i = 0; i < topics.size(); i++) {
        if (strcmp(topics[i].topic, searched) == 0) {
            return topics[i].sf;
        }
    }
    return -1;
}

void send_tcp(unordered_map<string, client_data> &clients, unordered_map<int, string> &connected_clients,
tcp_msg msg_tcp) {
    char the_topic[51];
    strcpy(the_topic, msg_tcp.topic);
    auto it = connected_clients.begin();
    while (it != connected_clients.end()) {
        if (clients.find(it->second) != clients.end()) {
            if (found_topic(clients[it->second].topics, the_topic) == 1) {
                int res = send_msg(it->first, (char *) &msg_tcp);
                DIE(res < 0, "Send failed");
            }
        }
        it++;
    }
    auto it2 = clients.begin();
    while (it2 != clients.end()) {
        if (it2->second.connected == 0 && found_topic(it2->second.topics, the_topic) == 1) {
            if (client_sf(it2->second.topics, the_topic) == 1) {
                it2->second.tcp_messages.push(msg_tcp);
            }
        }
        it2++;
    }
}

void close_all(int max_sock, fd_set read_fds) {
    int i;
    for (i = 2; i <= max_sock; i++) {
        if (FD_ISSET(i, &read_fds)) {
            close(i);
        }
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    setvbuf(stdin, NULL, _IONBF, BUFSIZ);
    if (argc != 2) {
        usage_server(argv[0]);
    }
    char buffer[BUFLEN];
    fd_set read_fds, tmp_fds;
    tcp_msg msg_tcp;
    udp_msg *msg_udp;
    msg_to_server *msg_for_server;
    socklen_t udp_len = sizeof(sockaddr);
    socklen_t clilen = sizeof(sockaddr);
    unordered_map<string, client_data> clients;
    unordered_map<int, string> connected_clients;
    int tcp_sock, udp_sock, flag_ngl = 1;
    int port = atoi(argv[1]);
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sock < 0, "initialise tcp_socket went wrong");
    udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
    DIE(udp_sock < 0, "initialise udp_socket went wrong");
    sockaddr_in tcp_addr, udp_addr, cli_addr;
    memset((char *) &tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(port);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    memset((char *) &udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(port);
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    int ret;
    ret = bind(tcp_sock, (struct sockaddr *) &tcp_addr, sizeof(sockaddr_in));
    DIE (ret < 0, "TCP bind failed");
    ret = bind(udp_sock, (struct sockaddr *) &udp_addr, sizeof(sockaddr_in));
    DIE (ret < 0, "UDP bind failed");
    ret = listen(tcp_sock, MAX_CLIENTS);
    DIE (ret < 0, "Listen failed");

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    FD_SET(tcp_sock, &read_fds);
    FD_SET(udp_sock, &read_fds);
    FD_SET(0, &read_fds);
    int max_tcp_sock = max(tcp_sock, udp_sock);
    int i, new_tcp_sock;
    int exit_program = 0;

    while (exit_program == 0) {
        tmp_fds = read_fds;
        memset(buffer, 0, BUFLEN);
        ret = select(max_tcp_sock + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "Select failed");

        for (i = 0; i <= max_tcp_sock; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == STDIN_FILENO) {
                    scanf("%s", buffer);
                    if (strncmp(buffer, "exit", 4) == 0) {
                        exit_program = 1;
                        break;
                    } else {
                        printf("Invalid command\n");
                    }
                }
                else if (i == tcp_sock) {
                    new_tcp_sock = accept(tcp_sock, (struct sockaddr *) &cli_addr, &clilen);
                    DIE(new_tcp_sock < 0, "Accept failed");

                    setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY,
                    (char *) &flag_ngl, sizeof(int));
                    FD_SET(new_tcp_sock, &read_fds);
                    if (new_tcp_sock > max_tcp_sock) {
                        max_tcp_sock = new_tcp_sock;
                    }
                    memset(buffer, 0, BUFLEN);
                    ret = recv(new_tcp_sock, buffer, BUFLEN - 1, 0);
                    DIE(ret < 0, "Receive failed");

                    string client_id(buffer);
                    auto found_client = clients.find(client_id);
                    if (found_client != clients.end()) {
                        if (found_client->second.connected == 1) {
                            FD_CLR(new_tcp_sock, &read_fds);
                            printf("Client %s already connected.\n", buffer);
                            close(new_tcp_sock);
                            FD_CLR(new_tcp_sock, &read_fds);
                        } else {
                            found_client->second.connected = 1;
                            connected_clients.insert({new_tcp_sock, client_id});
                            printf("New client %s connected from %s:%d.\n", buffer,
                            inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                            while(!found_client->second.tcp_messages.empty()) {
                                msg_tcp = found_client->second.tcp_messages.front();
                                ret = 0;
                                int stop = sizeof(tcp_msg);
                                char *buff_send = (char *)&msg_tcp;
                                while (stop > 0) {
                                    ret = send(new_tcp_sock, buff_send, sizeof(tcp_msg), 0);
                                    if (ret <= 0) {
                                        DIE(ret < 0, "Transmission failed");
                                        break;
                                    }
                                    buff_send += ret;
                                    stop -= ret;
                                }
                                found_client->second.tcp_messages.pop();
                            }

                        }
                    } else {
                        connected_clients.insert({new_tcp_sock, client_id});
                        client_data new_client;
                        new_client.connected = 1;
                        clients.insert({client_id, new_client});
                        printf("New client %s connected from %s:%d.\n", buffer,
                            inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                    }
                } else if (i == udp_sock) {
                    // printf("SKKDKK\n");
                    memset(buffer, 0, BUFLEN);
                    ret = recv(udp_sock, buffer, BUFLEN, 0);
                    DIE(ret < 0, "UDP error");
                    msg_udp = (udp_msg *) buffer;
                    memset(&msg_tcp, 0, sizeof(msg_tcp));
                    msg_tcp.port_client_udp = ntohs(udp_addr.sin_port);
                    strcpy(msg_tcp.ip_client_udp, inet_ntoa(udp_addr.sin_addr));
                    convert_to_tcp(msg_udp, &msg_tcp);
                    send_tcp(clients, connected_clients, msg_tcp);
                } else {
                    // primim mesaj de la subscriber
                    memset(buffer, BUFLEN, 0);
                    ret = recv(i, buffer, BUFLEN, 0);
                    string cli_id(connected_clients.at(i));
                    int cli_sock = i;
                    DIE(ret < 0, "Receive failed");
                    if (ret == 0) {
                        cout << "Client " << connected_clients.at(i) << " disconnected." << endl;
                        clients.at(connected_clients.at(i)).connected = 0;
                        connected_clients.erase(i);
                        FD_CLR(i, &read_fds);
                        close(i);
                    } else {
                        // Comanda de subcribe sau unsubscribe
                        msg_for_server = (msg_to_server *) buffer;
                        if (strcmp(msg_for_server->command, "subscribe") == 0) {
                            if (search_topic(clients.at(cli_id).topics, msg_for_server->topic) == -1) {
                                topic_struct new_topic;
                                new_topic.sf = msg_for_server->sf;
                                strcpy(new_topic.topic, msg_for_server->topic);
                                clients.at(cli_id).topics.push_back(new_topic);
                            }
                        } else if (strcmp(msg_for_server->command, "unsubscribe") == 0) {
                            int pos = search_topic(clients.at(cli_id).topics, msg_for_server->topic);
                            if (pos != -1) {
                                clients.at(cli_id).topics.erase(clients.at(cli_id).topics.begin() + pos);
                            }
                        }
                    }
                }
            }
        }
    }
    close_all(max_tcp_sock, read_fds);
    return 0;
}
