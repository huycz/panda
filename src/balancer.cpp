#include "balancer.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

void balancer::loop(){
    balancer bc;
    server *svr = 0;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1){
        printf("balancer create sock failed!(%s)\n", strerror(errno));
        return;
    }
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    addr.sin_port = htons(BALANCE_PORT);
    int rval = bind(sock, (sockaddr*)&addr, sizeof(addr));
    if(rval == -1){
        char ipbuf[64] = {0};
        printf("balancer bind sock to %s:%d failed!(%s)\n", inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf)), ntohs(BALANCE_PORT), strerror(errno));
        return;
    }
    while((svr = server::waiting_message(sock, bc)) != 0){
        svr->handle_message(bc);
    }
}

int main(){
    balancer::loop();
    return 0;
}

