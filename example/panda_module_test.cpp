/*
 * panda_module_test.cpp : this file will built as a so, we will run
 *                         panda server with it. Here, we only care about
 *                         input data and output data.
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


#include "st.h"
#include "panda_log.h"
#include "panda_procedure.h"
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif


#define USE_ST_SOCKET 0


    int set_none_blocking(int fd)
    {
        int ret = 0;
        int flags = 0;
        if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
            flags = 0;
        ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        if (ret < 0)
            perror("fcntl");

        return ret;
    }


    int get_time_from_local_udp(char *udp_time)
    {
        int sockfd,len,ret;
        struct sockaddr_in saddr;
        char buff[256];
#if USE_ST_SOCKET
        st_netfd_t nfd;
#endif
        struct servent *servinfo = NULL;
        struct hostent *hostinfo = NULL;



        char hname[] = "localhost";
        hostinfo = gethostbyname(hname);
        if(!hostinfo) {
            panda_log_info("No host: %s\n ",hname);
            exit(1);
        }
        panda_log_info("IP : %s\n", inet_ntoa(*(struct in_addr *) *(hostinfo->h_addr_list)));

        servinfo = getservbyname("daytime","udp");
        if(!servinfo) {
            panda_log_info("No day time server \n");
            exit(1);
        }
        panda_log_info("PORT : %d\n", ntohs(servinfo->s_port));

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1)
        {
            perror("socket");
        }
#if USE_ST_SOCKET
        nfd = st_netfd_open_socket(sockfd);
#else
        set_none_blocking(sockfd);
#endif
        saddr.sin_family = AF_INET;
        saddr.sin_port = servinfo->s_port;
        saddr.sin_addr = *(struct in_addr *) *(hostinfo->h_addr_list);
        len = sizeof(saddr);

        do {
#if USE_ST_SOCKET
            st_sendto(nfd, "", 1, (sockaddr *)&saddr, sizeof(struct sockaddr), 10);
            ret = st_recvfrom(nfd, buff, sizeof(buff), (sockaddr *)&saddr, &len, 10);
#else
            sendto(sockfd, "", 1, MSG_DONTWAIT, (struct sockaddr *)&saddr, (socklen_t)len);
            ret = recvfrom(sockfd, buff, sizeof(buff), MSG_DONTWAIT, (struct sockaddr *)&saddr, (socklen_t*)&len);
#endif
        } while (ret <= 0);
        buff[ret] = '\0';
        strcpy(udp_time, buff);
        close(sockfd);
        return 0;
    }


    int test_request(void *req_data, size_t count, void *pgdata)
    {
        char time_buf[64] = "";
        panda_log_info("req_data : %s\n", (char*)req_data);
        get_time_from_local_udp(time_buf);
        panda_log_info("get udp time : %s\n", time_buf);
        strncpy((char *)panda_procedure_group_data(pgdata), time_buf, sizeof(time_buf));
        panda_log_info("panda_procedure_group_data(pgdata) = %s\n", (char *)panda_procedure_group_data(pgdata));

        return 0;
    }


    int test_reply(void *rep_data, size_t *count, void *pgdata)
    {
        panda_log_trace();
        strcpy((char*)rep_data, (char *)panda_procedure_group_data(pgdata));
        *count = strlen((char *)panda_procedure_group_data(pgdata));

        panda_log_info("rep_data = %s, count = %u\n", (char *)rep_data, *count);
        return 0;
    }


    void panda_module_init(void *pgdata)
    {
        panda_procedure_t pro;
        pro.type = REQUEST | REPLY;
        pro.request = test_request;
        pro.reply = test_reply;

        panda_log_trace();
        panda_log_flush();
        panda_procedure_group_data_init(pgdata, malloc(128));
        panda_procedure_register(&pro, 1, pgdata);
        panda_procedure_commit(pgdata);
        panda_log_trace();
    }


    void panda_module_exit(void *pgdata)
    {
        panda_log_info("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
        free(panda_procedure_group_data(pgdata));
        panda_log_flush();
    }


#ifdef __cplusplus
}
#endif


