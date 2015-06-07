#ifndef _FORWARD_H_
#define _FORWARD_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../inc/panda.h"
#include "balancer.h"
#include <stdio.h>
class forward {
    enum {
        SYN,
        FIN,
        ACK,
        PACKET_SIZE = 4096,
        WINDOW_SIZE = 10,
    };
    balancer::tailang_client tailang;
    panda_attribute attr;
    sockaddr_in addr;
    int sock;
public:
    class panda_server {
        int sock;
        bool connected;
        sockaddr_in peer;
    public:
        panda_server(int sock):sock(sock),connected(false){}
        bool send(const void *data, int len);
        bool recv(void *data, int &len);
        bool disconnect();
        bool listen();
        bool transform();
        bool start(panda_alloc_func_t alloc, panda_free_func_t free, panda_request_func_t reuqest);
    };
    class panda_client {
        int sock;
        bool connected;
        const sockaddr_in peer;
    public:
        panda_client(int sock, sockaddr_in &peera):sock(sock),connected(false),peer(peera){}
        bool connect();
        bool disconnect();
        bool recv(void *data, int &len);
        bool send(const void *data, int len);
    };
    bool connect_balancer();
    bool poll();
};

#endif /* _FORWARD_H_ */
