#ifndef _BALANCER_H_
#define _BALANCER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <deque>
#include <list>
#include <map>
#include "../inc/panda.h"

class balancer {
    enum port {
        BALANCE_PORT = 6060,
    };
    enum message {
        DISCONNECT,
        PANDA_CONNECT,
        TAILANG_CONNECT,
        SUCCESS,    /* all packet sent, server need client's ACK */
        FAILURE,
        RETRANSMISSION,  /* if client find some packet drop, request server retry */
        PORT_ALLOC,  /* if tailang use it, mean request port, never free, panda will manager port */
        PORT_READY = PORT_ALLOC,   /* panda manager the ready port */
        PANDA_LOAD,
        MESSAGE_MAX
    };
    class server {
        struct packet {
            int len;
            const unsigned char data[0];
        };
        std::deque<const packet*> mq;
        virtual bool handling_message(const unsigned char *buf, int len, balancer &bc) = 0;
    public:
        sockaddr_in peer;
        int sock;
        server(sockaddr_in &peer, int sock):peer(peer), sock(sock){}
        enum {
            DISCONNECT, /* client disconnect */
            CONNECTED,  /* new client connected */
            PENDINGMSG, /* pending message need resolve */
        };
        static server *waiting_message(int sock, balancer &bc);
        virtual bool handle_message(balancer &bc){
            const packet *pk = mq.front();
            bool rval = handling_message(pk->data, pk->len, bc);
            delete pk;
            mq.pop_front();
            return rval;
        }
        virtual ~server(){}
    };
    std::list<server *> servlist;
    server *find_server(struct sockaddr_in &addr);
    struct attr_cmp {
        bool operator()(const panda_attribute &lhs, const panda_attribute &rhs) const {
            if(lhs.port < rhs.port){
                return true;
            }
            return (lhs.prot & rhs.prot) == 0;
        }
    };
    std::multimap<const panda_attribute, const sockaddr_in, attr_cmp> panda_skill;
    void regist_server(server *tailang);
    bool work_port_enable(const panda_attribute &attr, const sockaddr_in &addr);
    bool work_port_disable(panda_attribute &attr, sockaddr_in &addr);
public:
    class tailang_client { /* forward request panda info */
        sockaddr_in peer;
        int sock;
    public:
        bool connect(const char *ipstring);
        bool request(panda_attribute &attr, sockaddr_in &addr);
    };
    class tailang_server : public server {
        bool handling_message(const unsigned char *buf, int len, balancer &bc);
        tailang_server(sockaddr_in peer, int sock):server(peer,sock){}
    public:
        static tailang_server *maker(sockaddr_in &peer, int sock, const unsigned char *buf, int len);
    };

    class panda_client{  /* panda register and report self message */
        panda_attribute attr;
        sockaddr_in peer;
        int sock;
    public:
        bool connect(const char *ipstring, const panda_attribute &attr);
        bool report_port_ready(unsigned short port);
        /* cpurate is check local cpu, memoryrate check system wide */
        bool report_load(int cpurate, int memoryrate);
    };
    class panda_server : public server {
        panda_attribute attr;
        int load;
        bool handling_message(const unsigned char *buf, int len, balancer &bc);
        panda_server(panda_attribute attr, sockaddr_in peer, int sock, int load):
            server(peer,sock),attr(attr),load(load){}
    public:
        static panda_server *maker(sockaddr_in &peer, int sock, const unsigned char *buf, int len);
    };
    void update_panda_load(panda_server &panda){}
    static void loop();
};

#endif /* _BALANCER_H_ */
