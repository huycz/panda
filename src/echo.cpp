#include "../inc/panda.h"
#include <stdio.h>

int panda_init(panda_attribute *attr){
    attr->mode = ONE_SHORT;
    attr->prot = panda_service_prot_flag(PROT_UDP | PROT_TCP);
    attr->port = 2015;
    attr->id   = 1;
    printf("echo module inited\n");
    return 0;
}

void panda_destroy(int timeout){
    printf("echo module destroy\n");
}

int panda_request(const void *obj, int panda, const void *buf, int size, int flag, int (*reply)(const void *obj, int panda, const void *buf, int size)){
    reply(obj, panda, buf, size);
    return -1;
}

int panda_alloc(){
    return 0;
}

int panda_free(int panda){
    return 0;
}
