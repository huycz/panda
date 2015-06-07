#include "forward.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(){
    forward fw;
    fw.connect_balancer();
    fw.poll();
    return 0;
}

bool forward::connect_balancer(){
    tailang.connect("127.0.0.1");
    attr.port = 2015;
    attr.prot = panda_service_prot_flag(PROT_UDP);
    return true;
}

bool forward::poll(){
    while(1){
        if(!tailang.request(attr, addr)){
            printf("wait resource ready!\n");
            sleep(3);
            continue;
        }
        char ipbuf[64];
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if(sock == -1){
            printf("create udp sock failed!(%s)\n", strerror(errno));
            return false;
        }
        sockaddr_in local = {0};
        local.sin_family = AF_INET;
        int rval = bind(sock, (sockaddr*)&local, sizeof(local));
        if(rval == -1){
            printf("bind sock failed!(%s)\n", strerror(errno));
            return false;
        }

        panda_client panda(sock, addr);
        printf("request id:%d mode:%d localaddr:%s:%d\n", attr.id, attr.mode, inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf)), ntohs(addr.sin_port));
        panda.connect();
        char sbuf[256],rbuf[256];
        while(1){
            bzero(sbuf, sizeof(sbuf));
            bzero(rbuf, sizeof(rbuf));
            panda.send(sbuf, sprintf(sbuf, "hello~%d", rand()%256));
            int len = sizeof(rbuf);
            if(!panda.recv(rbuf, len)){
                break;
            }
            printf("service once:%s ~~ %s\n", sbuf, rbuf);
        }
    }
    return true;
}

