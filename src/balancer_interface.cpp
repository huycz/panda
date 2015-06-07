#include "balancer.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

balancer::server *balancer::find_server(sockaddr_in &addr){
    for(std::list<server*>::iterator it = servlist.begin();
            it != servlist.end(); ++it){
        if(memcmp(&(*it)->peer, &addr, sizeof(addr)) == 0){
            return *it;
        }
    }
    return 0;
}

void balancer::regist_server(server *srv){
    servlist.push_back(srv);
}

balancer::server *balancer::server::waiting_message(int sock, balancer &bc){
    while(1){
        sockaddr_in addr = {0};
        socklen_t len = sizeof(addr);
        unsigned char buf[256];
        int rval = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&addr, &len);
        if(rval == -1){
            printf("recvfrom client failed!(%s)\n", strerror(errno));
            return 0;
        }
        if(rval == 0){
            printf("impossable!!!\n");
            return 0;
        }
        unsigned short port = ntohs(addr.sin_port);
        if(*buf == PANDA_CONNECT){
            char ipbuf[64] = {0};
            printf("panda [%s:%d] connected\n", inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf)), port);
            bc.regist_server(panda_server::maker(addr, sock, buf, sizeof(buf)));
            continue;
        }else if(*buf == TAILANG_CONNECT){
            char ipbuf[64] = {0};
            printf("tailang [%s:%d] connected\n", inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf)), port);
            bc.regist_server(tailang_server::maker(addr, sock, buf, sizeof(buf)));
            continue;
        }else{
            server *nsvr = bc.find_server(addr);
            if(!nsvr){
                char ipbuf[64] = {0};
                printf("some guy [%s:%d] need connect first!\n", inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf)), port);
                char buf[1] = {FAILURE};
                sendto(sock, buf, 1, 0, (sockaddr*)&addr, len);
                continue;
            }
            packet *pk = reinterpret_cast<packet *>(new char [sizeof(packet)+rval]);
            pk->len = rval;
            memcpy((void*)pk->data, buf, rval);
            nsvr->mq.push_back(pk);
            return nsvr;
        }
    }
    return 0;
}

bool balancer::tailang_client::connect(const char *ipstring){
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1){
        printf("create tailang sock failed!(%s)\n", strerror(errno));
        return false;
    }
    sockaddr_in localaddr;
    localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = INADDR_ANY;
    int rval = bind(sock, (sockaddr*)&localaddr, sizeof(localaddr));
    if(rval == -1){
        printf("tailang bind localaddr failed!(%s)\n", strerror(errno));
        return false;
    }
    peer.sin_family = AF_INET;
    if(inet_pton(AF_INET, ipstring, &peer.sin_addr) != 1){
        printf("tailang conv addr:%s failed!(%s)\n", ipstring, strerror(errno));
        return false;
    }
    peer.sin_port = htons(BALANCE_PORT);
    char buf[] = {TAILANG_CONNECT};
    rval = sendto(sock, buf, 1, 0, (sockaddr*)&peer, sizeof(peer));
    if(rval == -1){
        printf("tailang connect to balancer failed!(%s)\n", strerror(errno));
        return false;
    }
    return true;
}

bool balancer::tailang_client::request(panda_attribute &attr, sockaddr_in &addr){
    unsigned char buf[32] = {PORT_ALLOC};
    buf[1] = attr.port >> 8;
    buf[2] = attr.port & 0xff;
    buf[3] = attr.prot;
    int rval = sendto(sock, buf, 4, 0, (sockaddr*)&peer, sizeof(peer));
    if(rval == -1){
        printf("send PORT_ALLOC failed!(%s)\n", strerror(errno));
        return false;
    }
    socklen_t len = sizeof(peer);
    rval = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&peer, &len);
    if(rval == -1){
        printf("recv PORT_ALLOC failed!(%s)\n", strerror(errno));
        return false;
    }
    if(*buf == FAILURE){
        printf("recv a balancer failed!(%s)\n", strerror(ntohl(*(int*)(buf+1))));
        return false;
    }
    attr.mode = panda_work_mode(buf[1]);
    attr.prot = panda_service_prot_flag(buf[2]);
    attr.port = buf[3] << 8 | buf[4];
    attr.id   = buf[5] << 8 | buf[6];
    memcpy(&addr, buf+7, sizeof(addr));
    return true;
}

balancer::tailang_server *balancer::tailang_server::maker(sockaddr_in &peer, int sock, const unsigned char *buf, int len){
    return new tailang_server(peer, sock);
}

bool balancer::tailang_server::handling_message(const unsigned char *data, int len, balancer &bc){
    switch(data[0]){
        case PORT_ALLOC:
        {
            panda_attribute attr;
            attr.port = data[1] << 8 | data[2];
            attr.prot = panda_service_prot_flag(data[3]);
            int rval = 0;
            sockaddr_in addr;
            if(bc.work_port_disable(attr, addr)){
                unsigned char buf[1+6+sizeof(addr)];
                buf[0] = SUCCESS;
                buf[1] = attr.mode;
                buf[2] = attr.prot;
                buf[3] = attr.port >> 8;
                buf[4] = attr.port & 0xff;
                buf[5] = attr.id >> 8;
                buf[6] = attr.id & 0xff;
                memcpy(buf+7, &addr, sizeof(addr));
                rval = sendto(sock, buf, sizeof(buf), 0, (sockaddr*)&peer, sizeof(peer));
            }else{
                unsigned char buf[5] = {FAILURE};
                *(int*)(buf+1) = htonl(EBUSY);
                rval = sendto(sock, buf, sizeof(buf), 0, (sockaddr*)&peer, sizeof(peer));
            }
            if(rval == -1){
                bc.work_port_enable(attr, addr);
                printf("send message failed!(%s)\n", strerror(errno));
                return false;
            }
            return true;
        }
        default:
            break;
    }
    return false;
}

bool balancer::panda_client::connect(const char *ipstring, const panda_attribute &attr){
    printf("panda_server id: %d, port %d, prot: %d, mode: %d\n", attr.id, attr.port, attr.prot, attr.mode);
    int rval = 0;
    while(1){
        sock = socket(PF_INET, SOCK_DGRAM, 0);
        if(sock == -1){
            printf("create panda sock failed!(%s)\n", strerror(errno));
            return false;
        }
        sockaddr_in localaddr = {0};
        localaddr.sin_family = AF_INET;
        rval = bind(sock, (sockaddr*)&localaddr, sizeof(localaddr));
        if(rval == -1){
            printf("panda bind localaddr failed!(%s)\n", strerror(errno));
            close(sock);
            sleep(2);
            continue;
        }
        break;
    }
    bzero(&peer, sizeof(peer));
    peer.sin_family = AF_INET;
    peer.sin_port = htons(BALANCE_PORT);
    if(inet_pton(AF_INET, ipstring, &peer.sin_addr) != 1){
        printf("panda conv addr:%s failed!(%s)\n", ipstring, strerror(errno));
        return false;
    }
    char buf[1+6] = {PANDA_CONNECT};
    buf[1] = attr.mode;
    buf[2] = attr.prot;
    buf[3] = attr.port >> 8;
    buf[4] = attr.port & 0xff;
    buf[5] = attr.id >> 8;
    buf[6] = attr.id & 0xff;
    char ipbuf[64] = {0};
    printf("connect to balancer %s:%d...\n", inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf)), ntohs(peer.sin_port));
    rval = sendto(sock, buf, sizeof(buf), 0, (sockaddr*)&peer, sizeof(peer));
    if(rval == -1){
        printf("panda connect to balancer failed!(%s)\n", strerror(errno));
        return false;
    }
    return true;
}

balancer::panda_server *balancer::panda_server::maker(sockaddr_in &peer, int sock, const unsigned char *buf, int len){
    panda_attribute attr;
    attr.mode = panda_work_mode(buf[1]);
    attr.prot = panda_service_prot_flag(buf[2]);
    attr.port = buf[3] << 8 | buf[4];
    attr.id   = buf[5] << 8 | buf[6];
    printf("create panda_server id: %d, port %d, prot: %d, mode: %d\n", attr.id, attr.port, attr.prot, attr.mode);
    return new panda_server(attr, peer, sock, 0);
}

bool balancer::panda_client::report_port_ready(unsigned short port){
    char buf[1+2] = {PORT_READY};
    buf[1] = port >> 8;
    buf[2] = port & 0xff;
    char ipbuf[64] = {0};
    printf("send port %d ready to %s:%d...\n", ntohs(port), inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf)), ntohs(peer.sin_port));
    int rval = sendto(sock, buf, sizeof(buf), 0, (sockaddr*)&peer, sizeof(peer));
    if(rval == -1){
        printf("send port ready failed!(%s)\n", strerror(errno));
        return false;
    }
    return true;
}

bool balancer::panda_client::report_load(int cpurate, int memoryrate){
    char buf[1+1+1] = {PANDA_LOAD, cpurate&0xff, memoryrate&0xff};
    int rval = sendto(sock, buf, sizeof(buf), 0, (sockaddr*)&peer, sizeof(peer));
    if(rval == -1){
        printf("send panda load failed!(%s)\n", strerror(errno));
        return false;
    }
    return true;
}

bool balancer::panda_server::handling_message(const unsigned char *data, int len, balancer &bc){
    switch(data[0]){
        case PORT_READY:
        {
            unsigned short port = data[1] << 8 | data[2];
            sockaddr_in naddr = peer;
            naddr.sin_port = port;
            bc.work_port_enable(attr, naddr);
            return true;
        }
        case PANDA_LOAD:
        {
            bc.update_panda_load(*this);
            return true;
        }
        default:
            break;
    }
    return false;
}

bool balancer::work_port_enable(const panda_attribute &attr, const sockaddr_in &addr){
    char buf[64];
    printf("enable: %d %d %d %d %s:%d\n", attr.id, attr.port, attr.prot, attr.mode, inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf)), ntohs(addr.sin_port));
    panda_skill.insert(std::make_pair(attr, addr));
    return true;
}

bool balancer::work_port_disable(panda_attribute &attr, sockaddr_in &addr){
    printf("request: %d, %d\n", attr.port, attr.prot);
    std::multimap<const panda_attribute, const sockaddr_in, attr_cmp>::iterator iter = panda_skill.find(attr);
    if(iter != panda_skill.end()){
        attr.id = iter->first.id;
        attr.mode = iter->first.mode;
        addr = iter->second;
        char buf[64];
        printf("disable %d %d %d %d, %s:%d", attr.id, attr.port, attr.prot, attr.mode, inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf)), ntohs(addr.sin_port));
        panda_skill.erase(iter);
        return true;
    }
    printf("have no support port to use!\n");
    return false;
}
