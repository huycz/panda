#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <assert.h>
#include <vector>
#include <queue>
#include <pthread.h>
#include "forward.h"
#include "balancer.h"
#include "../inc/panda.h"  /* dragon warrior is a panda */

static void *thread_create(void *arg);
class service;

class thread_control_block {
    service *svc;
    int sock;
    unsigned short port;
    panda_alloc_func_t alloc;
    panda_free_func_t free;
    panda_request_func_t request;
    sem_t actived;
public:
    thread_control_block(service *svc,
            panda_alloc_func_t alloc,
            panda_free_func_t free,
            panda_request_func_t request):
        svc(svc),sock(-1),port(0),alloc(alloc),free(free),request(request){
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if(sock == -1){
            printf("open sock failed!(%s)\n", strerror(errno));
            exit(-1);
        }
        sockaddr_in servaddr = {0};
        servaddr.sin_family = AF_INET;
        int rval = bind(sock, (sockaddr*)&servaddr, sizeof(servaddr));
        if(rval == -1){
            printf("bind port failed!(%s)\n", strerror(errno));
            exit(-1);
        }
        socklen_t len = sizeof(servaddr);
        rval = getsockname(sock, (sockaddr*)&servaddr, &len);
        if(rval == -1){
            printf("get sockfailed!(%s)\n", strerror(errno));
            close(sock);
            exit(-1);
        }
        char ipstring[64];
        port = servaddr.sin_port;
        sem_init(&actived, 0, 0);
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_DETACHED);
        rval = pthread_create(&tid, &attr, thread_create, this);
        pthread_attr_destroy(&attr);
        if(rval != 0){
            printf("create service thread failed!(%s)\n", strerror(errno));
            exit(-1);
        }
    }
    bool needActive(){
        int v;
        sem_getvalue(&actived, &v);
        return v == 0;
    }
    bool resume(){
        sem_post(&actived);
        return true;
    }
    bool run();
};

class service {
    enum {
        SERVICE_COUNT_MAX = 4,
    };
    balancer::panda_client balancer_client;
    std::deque<thread_control_block*> threads; /* head active, tail deactive */
    sem_t service_count; /* thread max count */
public:
    service():balancer_client(){
        sem_init(&service_count, 0, SERVICE_COUNT_MAX);
    }
    bool connect_balancer(int cpuid, const char *addr, const panda_attribute &attr,
            panda_alloc_func_t alloc, panda_free_func_t free, panda_request_func_t request){
        for(int i = 0; i < SERVICE_COUNT_MAX; ++i){
            threads.push_back(new thread_control_block(this, alloc, free, request));
        }
        return balancer_client.connect(addr, attr);
    }
    bool deactive(thread_control_block *tcb){
        threads.erase(std::find(threads.begin(), threads.end(), tcb));
        threads.push_back(tcb);
        sem_post(&service_count);
        int v;
        sem_getvalue(&service_count, &v);
        return true;
    }
    bool port_ready(unsigned short port){
        return balancer_client.report_port_ready(port);
    }
    bool working(){
        while(1){
            timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;
            int v;
            sem_getvalue(&service_count, &v);
            while(sem_timedwait(&service_count, &ts) != -1){
                if(threads.back()->needActive()){
                    threads.push_front(threads.back());
                    threads.pop_back();
                    threads.front()->resume();
                }
            }
            //balancer_client.report_load(0, 0);
        }
        return true;
    }
};

bool thread_control_block::run(){
    while(1){
        sem_wait(&actived);
        if(!svc->port_ready(port)){
            printf("can't connect balance!\n");
        }else{
            forward::panda_server server(sock);
            server.start(alloc, free, request);
        }
        sleep(1);
        svc->deactive(this);
    }
    return true;
}

/*
 * argv[0] progname
 * argv[1] cpuid
 * argv[2] libname
 * argv[3] balancer ip
 * argv[4] NULL
 */
int main(int argc, char *argv[]){
    sigset_t ub;
    sigemptyset(&ub);
    sigaddset(&ub, SIGINT);
    sigaddset(&ub, SIGBUS);
    sigaddset(&ub, SIGABRT);
    sigaddset(&ub, SIGSEGV);
    sigaddset(&ub, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &ub, NULL);
    signal(SIGCHLD, SIG_IGN);
    void *dl = dlopen(argv[2], RTLD_LAZY);
    if(dl == NULL){
        printf("open solib:%s failed!\n", argv[1]);
        exit(-1);
    }
    printf("load lib\n");
    panda_init_func_t init =
        reinterpret_cast<panda_init_func_t>(dlsym(dl, "panda_init"));
    if(!init){
        printf("load init failed!(%s)\n", dlerror());
        exit(-1);
    }
    panda_destroy_func_t destroy =
        reinterpret_cast<panda_destroy_func_t>(dlsym(dl, "panda_destroy"));
    if(!destroy){
        printf("load destroy failed!(%s)\n", dlerror());
        exit(-1);
    }
    panda_request_func_t request =
        reinterpret_cast<panda_request_func_t>(dlsym(dl, "panda_request"));
    if(!request){
        printf("load request failed!(%s)\n", dlerror());
        exit(-1);
    }
    panda_alloc_func_t alloc =
        reinterpret_cast<panda_alloc_func_t>(dlsym(dl, "panda_alloc"));
    if(!alloc){
        printf("load alloc failed!(%s)\n", dlerror());
        exit(-1);
    }
    panda_free_func_t free =
        reinterpret_cast<panda_free_func_t>(dlsym(dl, "panda_free"));
    if(!free){
        printf("load free failed!(%s)\n", dlerror());
        exit(-1);
    }
    printf("lib init\n");
    panda_attribute attr;
    if(init(&attr) != 0){
        printf("panda_init failed!\n");
    }
    printf("start svc\n");
    service svc;
    if(!svc.connect_balancer(atoi(argv[1]), argv[3], attr, alloc, free, request)){
        printf("self register failed!\n");
        destroy(0);
        pause();
    }
    /* have work done condition? */
    svc.working();
    destroy(1000);
    return 0;
}

void *thread_create(void *arg){
    thread_control_block *tcb = reinterpret_cast<thread_control_block*>(arg);
    tcb->run();
    return arg;
}

