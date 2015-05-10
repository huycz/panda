/*
 * main.cpp : start monitor and working node
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include "iniparser.h"
#include "panda_log.h"
#include "panda_debug.h"
#include "panda_procedure.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>


#ifdef __cplusplus
extern "C" {
#endif


#if 1	//local data type
    typedef struct panda_balancer_config_s {
        char ip[16];
        uint16_t port;
    } panda_balancer_config_t;



    typedef struct panda_monitor_config_s {
        int32_t daemon;
        uint32_t nodes;
        char log_path[128];
    } panda_monitor_config_t;


    typedef struct panda_node_config_s {
        uint32_t heartbeat;
        char log_path[128];
        uint32_t request_data_size;
        uint32_t reply_data_size;
    } panda_node_config_t;
#endif	//local data type


#if 1	//local global configuration data, store
    static char config_path[128] __attribute__((section (".config_data")));
    static char module_path[128] __attribute__((section (".config_data")));
    static panda_balancer_config_t balancer_config __attribute__((section (".config_data")));
    static panda_monitor_config_t monitor_config __attribute__((section (".config_data")));
    static panda_node_config_t node_config __attribute__((section (".config_data")));
#endif


#if 1	//local functions
    void usage();
    void init_config(char *config_path);
    void init_module(char *module_path);
    void init_cmdline(int argc, char *argv[]);
    void debug_signal_handler(int sig);
    void child_signal_handler(int sig);
    void init_sighandler();
    void init_module_name(char *name, size_t n);
    void init_daemon();
    void init_one_working_node();
    void init_working_nodes();
#endif


    void usage()
    {
        //here we use printf, because panda log has not been initialized
        printf("Usage :\n");
        printf("In fact, most configuration is loaded by configuration file\n");
        printf("-c config-file : configuration for current module, default configuration : config.ini\n");
        printf("-m module.so   : module for current module, default module : module.so\n\n");

        exit(0);
    }


    void init_config(char *config_path)
    {
        dictionary *config_dir = NULL;

        config_dir = iniparser_load(config_path);
        if (config_dir == NULL)
            usage();

        //load balancer config
        panda_log_info("%s : load balancer config from %s ...\n", __FUNCTION__, config_path);
        strncpy(balancer_config.ip, iniparser_getstring(config_dir, "balancer:ip", (char*)""), sizeof(balancer_config.ip));
        assert(balancer_config.ip[0] != '\0');
        balancer_config.port = iniparser_getint(config_dir, "balancer:port", 0);
        assert(balancer_config.port != 0);

        panda_log_info("ip : %s, port : %u\n\n", balancer_config.ip, balancer_config.port);

        //load monitor config
        panda_log_info("%s : load monitor config from %s ...\n", __FUNCTION__, config_path);
        monitor_config.daemon = iniparser_getboolean(config_dir, "monitor:daemon", 1);
        monitor_config.nodes = iniparser_getint(config_dir, "monitor:nodes", 0);
        if (monitor_config.nodes == 0) {
            long processors = sysconf(_SC_NPROCESSORS_CONF);
            assert(processors > 0);
            monitor_config.nodes = (uint32_t)processors;
        }
        strncpy(monitor_config.log_path, iniparser_getstring(config_dir, "monitor:log_path", (char*)"./"), sizeof(monitor_config.log_path));

        panda_log_info("daemon : %d, nodes : %d, log_path : %s\n\n", monitor_config.daemon, monitor_config.nodes, monitor_config.log_path);

        //load node config
        panda_log_info("%s : load node config from %s ...\n", __FUNCTION__, config_path);
        node_config.heartbeat = (uint32_t)iniparser_getint(config_dir, "node:heartbeat", 60);
        strncpy(node_config.log_path, iniparser_getstring(config_dir, "node:log_path", (char*)"./"), sizeof(node_config.log_path));
        node_config.request_data_size = (uint32_t)iniparser_getint(config_dir, "node:request_data_size", DEFAULT_REQUEST_DATA_SIZE);
        node_config.reply_data_size = (uint32_t)iniparser_getint(config_dir, "node:reply_data_size", DEFAULT_REPLY_DATA_SIZE);

        panda_log_info("heartbeat : %u, log_path : %s, request_data_size = %u, reply_data_size = %u\n\n", node_config.heartbeat, node_config.log_path, node_config.request_data_size, node_config.reply_data_size);

        iniparser_freedict(config_dir);
    }


    void init_module(char *module_path)
    {
        void *module_handle = NULL;
        void (*panda_procedure_init)(void *data) = NULL;
        void (*panda_procedure_exit)(void *data) = NULL;
        panda_log_info("module : %s\n", module_path);

        module_handle = dlopen(module_path, RTLD_GLOBAL | RTLD_LAZY);
        if (module_handle == NULL)
            panda_log_fatal("open %s fail\n", module_path);

        panda_procedure_init = (void (*)(void*))dlsym(module_handle, (char*)"panda_module_init");
        if (panda_procedure_init == NULL)
            panda_log_fatal("panda_module_init is NULL\n");
        panda_procedure_exit = (void (*)(void*))dlsym(module_handle, (char*)"panda_module_exit");
        if (panda_procedure_exit == NULL)
            panda_log_fatal("panda_module_exit is NULL\n");

        panda_procedure_group_init(panda_procedure_init, panda_procedure_exit);
    }


    void init_cmdline(int argc, char *argv[])
    {

        int cmd_opt = 0;
        strncpy(config_path, "./config.ini", sizeof(config_path));
        strncpy(module_path, "./module.so", sizeof(module_path));

        while ((cmd_opt  =  getopt(argc, argv, "c:m:")) != EOF) {
            switch (cmd_opt) {
            case 'c' :
                if (optarg)
                    strncpy(config_path, optarg, sizeof(config_path));
                else
                    usage();

                break;

            case 'm' :
                if (optarg)
                    strncpy(module_path, optarg, sizeof(module_path));
                else
                    usage();

                break;

            default :
                usage();
                break;
            }
        }
    }


    void debug_signal_handler(int sig)
    {
        panda_log_info("fatal error :\n");
        panda_log_info("recieve signal %d\n", sig);
        panda_log_flush();
        //you may see so many VMAs in maps, but trust me, it's just ST's cache, NOT memory leak
        panda_show_maps();
        panda_log_flush();
        //you may see some ?? in bt, because of using ST, esp/rsp has been changed
        panda_show_backtrace();
        panda_log_flush();

        signal(sig, SIG_DFL);
        raise(sig);
    }


    void child_signal_handler(int sig)
    {
        int status = 0;
        pid_t pid = 0;

        pid = waitpid(-1, &status, 0);
        panda_log_info("fatal error :\n");
        panda_log_info("pid : %u has been dead with status : %d\n", pid, status);

        init_one_working_node();
    }


    void init_sighandler()
    {
        uint32_t i = 0;
        int32_t dbg_sigs[] = {SIGINT, SIGABRT, SIGBUS, SIGFPE, SIGSEGV};
        int32_t ign_sigs[] = {SIGPIPE};

        for (i = 0; i < sizeof(dbg_sigs)/sizeof(dbg_sigs[0]); i++)
            if (signal(dbg_sigs[i], debug_signal_handler) == SIG_ERR)
                panda_log_error("init_sighandler %d error\n", dbg_sigs[i]);

        for (i = 0; i < sizeof(ign_sigs)/sizeof(ign_sigs[0]); i++)
            if (signal(ign_sigs[i], SIG_IGN) == SIG_ERR)
                panda_log_error("init_sighandler %d error\n", ign_sigs[i]);

        if (signal(SIGCHLD, child_signal_handler) == SIG_ERR)
            panda_log_error("init_sighandler %d error\n", SIGCHLD);
    }


    void init_module_name(char *name, size_t n)
    {
        char *last = module_path;
        char *last_but_one = NULL;

        //find the last '/'
        while ((last = strstr(last, (char*)"/")) != NULL) {
            last_but_one = ++last;
        }

        strncpy(name, last_but_one, n);
        if ((name = strstr(name, (char*)".")) != NULL)
            *name = '\0';
    }


    void init_daemon()
    {
        char name[16] = "";	//because PR_SET_NAME : The  name can be up to 16 bytes long
        char log_name[sizeof(node_config.log_path)] = "";
        char log_path[sizeof(node_config.log_path)] = "";

        if (monitor_config.daemon)
            if (daemon(1, 1) < 0)	//not change working directory and stdio
                panda_log_fatal("daemon fail");

        if (prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0) == -1)
            panda_log_fatal("prctl fail");

        memset(name, 0x00, sizeof(name));
        init_module_name(name, sizeof(name));
        name[13] = '\0';
        strcat(name, "_m");
        panda_log_info("init monitor name : %s\n", name);

        memcpy(log_path, monitor_config.log_path, sizeof(node_config.log_path));
        strncat(log_path, "/", sizeof(log_path));
        strncat(log_path, name, sizeof(log_path));
        strncat(log_path, "_%u", sizeof(log_path));
        snprintf(log_name, sizeof(log_path), log_path, getpid());
        panda_log_info("init monitor log name : %s\n", log_name);
        if (panda_log_init(log_name) < 0)
            panda_log_fatal("panda_log_init fail\n");

        panda_log_info("init daemon done\n");
    }


    void init_one_working_node()
    {
        pid_t pid = 0;
        char name[16] = "";	//because PR_SET_NAME : The  name can be up to 16 bytes long
        char log_name[sizeof(node_config.log_path)] = "";
        char log_path[sizeof(node_config.log_path)] = "";

        pid = fork();
        if (pid < 0)
            panda_log_fatal("fork fail\n");

        if (pid > 0) {
            return;
        }

        memset(name, 0x00, sizeof(name));
        init_module_name(name, sizeof(name));
        name[13] = '\0';
        strcat(name, "_n");
        panda_log_info("init working node name : %s\n", name);

        memcpy(log_path, node_config.log_path, sizeof(node_config.log_path));
        strncat(log_path, "/", sizeof(log_path));
        strncat(log_path, name, sizeof(log_path));
        strncat(log_path, "_%u", sizeof(log_path));
        snprintf(log_name, sizeof(log_path), log_path, getpid());
        if (panda_log_init(log_name) < 0)
            panda_log_fatal("panda_log_init fail\n");


        if (prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0) == -1)
            panda_log_fatal("prctl fail");

        panda_log_info("working node %u start\n", getpid());

        panda_procedure_set_data_size(node_config.request_data_size, node_config.reply_data_size);

        //TODO main processing loop
        panda_procedure_loop();
    }


    void init_working_nodes()
    {
        uint32_t i = 0;

        for (i = 0; i < monitor_config.nodes; i++) {
            init_one_working_node();
        }
    }


    int main(int argc, char *argv[])
    {
        init_cmdline(argc, argv);
        init_config(config_path);
        init_module(module_path);
        init_sighandler();
        init_daemon();
        init_working_nodes();

        //TODO init st, and enter main loop
        while(1)
            pause();

        return 0;
    }


#ifdef __cplusplus
}
#endif


