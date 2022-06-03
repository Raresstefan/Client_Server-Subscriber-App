#ifndef _HELPERS_H
#define _HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <cmath>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <string>
#include <iostream>

#define MAXRECV sizeof(tcp_msg)
#define BUFLEN 2000 // dimensiunea maxima a calupului de date
#define MAX_CLIENTS 300 // numarul maxim de clienti in asteptare

using namespace std;

struct __attribute__((packed)) udp_msg {
    char topic[50];
    uint8_t type;
    char msg[1501];
};

struct __attribute__((packed)) msg_to_server {
    char topic[51];
    char command[12];
    int sf;
};

struct __attribute__((packed)) tcp_msg {
    char ip_client_udp[16];
    uint16_t port_client_udp;
    char topic[51];
    char type[11];
    char msg[1501];
};

struct __attribute__((packed)) topic_struct {
    char topic[51];
    int sf;
};

struct __attribute__((packed)) client_data {
    vector<topic_struct> topics;
    int connected;
    queue<tcp_msg> tcp_messages;
};

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description) \
        do { \
        if (assertion) { \
        fprintf(stderr, "(%s, %d): ", \
        __FILE__, __LINE__); \
        perror(call_description); \
        exit(EXIT_FAILURE); \
        } \
    } while(0)

#endif