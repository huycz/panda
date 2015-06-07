#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/cpuset.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

static const char *dragon_warrior = "./src/dragon_warrior";

static int get_ncpu(){
	int mib[2] = {
		CTL_HW, HW_NCPU
	};
	int ncpu = 0;
	int rval = 0;
    size_t len = sizeof(ncpu);
	rval = sysctl(mib, 2, &ncpu, &len, NULL, 0);
	if(rval != 0){
		printf("get cpu number failed!(%s)\n", strerror(errno));
		return -1;
	}
	return ncpu;
}

static pid_t fork_warrior(int cpuid){
    pid_t po = fork();
    if(po == 0){  /* Po, the dragon warrior */
        cpuset_t cs;
        CPU_ZERO(&cs);
        CPU_SET(cpuid, &cs);
        int rval = cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, getpid(), sizeof(cs), &cs);
        if(rval == -1){
            printf("set pid:%d -> cpu:%d failed!(%s)\n", getpid(), cpuid, strerror(errno));
        }
        char buf[64];
        sprintf(buf, "%d", cpuid);
        if(execlp(dragon_warrior, "panda", buf, "./src/libecho.so", "127.0.0.1", NULL) == -1){
            printf("create dragon warrior(%s) failed!(%s)\n", dragon_warrior, strerror(errno));
            assert(0);
        }
    }
    return po;
}

static void save_po(int *pids, int len){
    int rval = 0;
	while((rval = waitpid(0, NULL, WNOHANG)) != 0){
		if(rval == -1){
			printf("wait warrior failed!(%s)\n", strerror(errno));
		}else{
            int cpuid = -1;
            for(int i = 0; i < len; ++i){
                if(pids[i] == rval){
                    cpuid = i;
                }
            }
            if(cpuid == -1){
                printf("can't find pid:%d, amazing!\n", rval);
                assert(0);
            }
            pids[cpuid] = fork_warrior(cpuid);
		}
	}
}

static void sig_pad(int){}

int main(int argc, char *argv[]){
	sigset_t all;
	sigfillset(&all);
	sigprocmask(SIG_BLOCK, &all, NULL);
    signal(SIGCHLD, sig_pad);
	while(1){
		pid_t xifu = fork();
		if(xifu){ /* wugui's process */
            sigdelset(&all, SIGCHLD);
            sigdelset(&all, SIGINT);
			sigsuspend(&all);
			int rval = waitpid(xifu, NULL, 0);
			if(rval == -1){
				printf("wait %d failed!(%s)\n", xifu, strerror(errno));
			}
			/* FIXME: xifu's po maybe aliving happy, need take care of them */
		}else{
			int ncpu = get_ncpu();
            int pids[ncpu];
			for(int i = 0; i < ncpu; ++i){
                pids[i] = fork_warrior(i);
			}
            sigdelset(&all, SIGCHLD);
            sigdelset(&all, SIGINT);
            while(1){
                sigsuspend(&all);
                save_po(pids, ncpu);
            }
		}
	}
	return 0;
}

