#include "forward.h"
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

bool forward::panda_client::connect(){
    if(!connected){
        unsigned char buf[] = {0xff, SYN};
        char ipstring[64];
        int rval = sendto(sock, buf, sizeof(buf), 0, (sockaddr*)&peer, sizeof(peer));
        if(rval == -1){
            printf("send SYN failed!(%s)\n", strerror(errno));
            return false;
        }
        socklen_t len = sizeof(peer);
        rval = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&peer, &len);
        if(rval == -1){
            printf("recv ACK failed!(%s)\n", strerror(errno));
            return false;
        }
        if(rval == 2 && buf[0] == 0xff && buf[1] == ACK){
            printf("panda server connected [%s:%d]\n", inet_ntop(AF_INET, &peer.sin_addr, ipstring, sizeof(ipstring)), ntohs(peer.sin_port));
            connected = true;
            return true;
        }
        printf("recv %d bytes\n", rval);
    }
    return false;
}

bool forward::panda_client::disconnect(){
    if(connected){
        connected = false;
        unsigned char buf[] = {0xff, FIN};
        int rval = sendto(sock, buf, sizeof(buf), 0, (sockaddr*)&peer, sizeof(peer));
        if(rval == -1){
            printf("send FIN failed!(%s)\n", strerror(errno));
            return false;
        }
    }
    return true;
}

bool forward::panda_client::recv(void *buf, int &len){
    if(connected){
        socklen_t slen = sizeof(peer);
        int rval = recvfrom(sock, buf, len, 0, (sockaddr*)&peer, &slen);
        if(rval == -1){
            char buf[64];
            printf("recv from [%s:%d] failed!(%s)\n", inet_ntop(AF_INET, &peer.sin_addr, buf, sizeof(buf)), ntohs(peer.sin_port), strerror(errno));
            connected = false;
            return false;
        }
        if(rval == 2 && *(unsigned char *)buf == 0xff && *((unsigned char*)buf+1) == FIN){
            char buf[64];
            printf("disconnect by peer [%s:%d]\n", inet_ntop(AF_INET, &peer.sin_addr, buf, sizeof(buf)), ntohs(peer.sin_port));
            connected = false;
            return false;
        }
        len = rval;
        return true;
    }
    printf("please connect first to recv\n");
    return false;
}

bool forward::panda_client::send(const void *data, int len){
    if(connected){
        char buf[64];
        printf("send to %d bytes [%s:%d]...\n", len, inet_ntop(AF_INET, &peer.sin_addr, buf, sizeof(buf)), ntohs(peer.sin_port));
        int rval = sendto(sock, data, len, 0, (sockaddr*)&peer, sizeof(peer));
        if(rval == -1){
            printf("send to [%s:%d] failed!(%s)\n", inet_ntop(AF_INET, &peer.sin_addr, buf, sizeof(buf)), ntohs(peer.sin_port), strerror(errno));
            return false;
        }
        return true;
    }
    printf("please connect first to send!\n");
    return false;
}

bool forward::panda_server::send(const void *data, int len){
    if(connected){
        int rval = sendto(sock, data, len, 0, (sockaddr*)&peer, sizeof(peer));
        if(rval == -1){
            char buf[64];
            printf("send to [%s:%d] failed!(%s)\n", inet_ntop(AF_INET, &peer.sin_addr, buf, sizeof(buf)), ntohs(peer.sin_port), strerror(errno));
            connected = false;
            return false;
        }
        return true;
    }
    printf("please connect first!\n");
    return false;
}

bool forward::panda_server::recv(void *buf, int &len){
    if(connected){
        socklen_t slen = sizeof(peer);
        char ipbuf[64];
        int rval = recvfrom(sock, buf, len, 0, (sockaddr*)&peer, &slen);
        if(rval == -1){
            printf("recv from [%s:%d] failed!(%s)\n", inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf)), ntohs(peer.sin_port), strerror(errno));
            connected = false;
            return false;
        }
        if(rval == 2 && *(unsigned char *)buf == 0xff && *((unsigned char*)buf+1) == FIN){
            /* disconnect by peer */
            char buf[64];
            printf("disconnect by peer [%s:%d]\n", inet_ntop(AF_INET, &peer.sin_addr, buf, sizeof(buf)), ntohs(peer.sin_port));
            connected = false;
            return false;
        }
        len = rval;
        return true;
    }
    return false;
}

bool forward::panda_server::disconnect(){
    if(connected){
        connected = false;
        unsigned char buf[] = {0xff, FIN};
        int rval = sendto(sock, buf, 2, 0, (sockaddr*)&peer, sizeof(peer));
        if(rval == -1){
            char buf[64];
            printf("disconnect with [%s:%d] failed!(%s)\n", inet_ntop(AF_INET, &peer.sin_addr, buf, sizeof(buf)), ntohs(peer.sin_port), strerror(errno));
        }
    }
    return true;
}

static int panda_reply(const void *obj, int panda, const void *buf, int size){
    forward::panda_server *serv = reinterpret_cast<forward::panda_server*>(const_cast<void*>(obj));
    if(serv->send(buf, size)){
        return 0;
    }
    printf("send error, closed\n");
    return -1;
}

bool forward::panda_server::listen(){
    if(!connected){
        unsigned char buf[64];
        socklen_t len = sizeof(peer);
        char ipbuf[64];
        printf("wait for connect.. %d\n", sock);
        int rval = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&peer, &len);
        if(rval == -1){
            char buf[64];
            printf("recv from [%s:%d] failed!(%s)\n", inet_ntop(AF_INET, &peer.sin_addr, buf, sizeof(buf)), ntohs(peer.sin_port), strerror(errno));
            return false;
        }
        if(!(rval == 2 && buf[0] == 0xff && buf[1] == SYN)){
            printf("recv wrong header!(0x%02x 0x%02x)\n", buf[0], buf[1]);
            return false;
        }
        printf("some one connect..\n", inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf)), ntohs(peer.sin_port));
        buf[0] = 0xff;
        buf[1] = ACK;
        rval = sendto(sock, buf, 2, 0, (sockaddr*)&peer, sizeof(peer));
        if(rval == -1){
            printf("send ACK failed!(%s)\n", strerror(errno));
            return false;
        }
        printf("panda_client [%s, %d] connected!\n", inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf)), ntohs(peer.sin_port));
        connected = true;
        return true;
    }
    return false;
}

bool forward::panda_server::start(panda_alloc_func_t alloc, panda_free_func_t free, panda_request_func_t request){
    int panda = alloc();
    while(listen()){
        char buf[1024];
        int len = sizeof(buf);
        if(recv(buf, len)){
            if(request(this, panda, buf, len, 0, panda_reply) == -1){
                disconnect();
                printf("disconnect with peer.\n");
                break;
            }
        }
    }
    free(panda);
    return true;
}

