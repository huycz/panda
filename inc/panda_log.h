#ifndef __PANDA_LOG_H__
#define __PANDA_LOG_H__


/*
 * panda_log.h : log redirection and log level
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


#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>


#ifdef __cplusplus
extern "C" {
#endif


    inline int panda_log_init(char *redirection_path)
    {
        fflush(stdout);
        fflush(stderr);
        if (redirection_path == NULL) {
            if (freopen("/dev/null", "a", stdout) == NULL) {
                perror("freopen stdout");
                return -errno;
            }

            if (freopen("/dev/null", "a", stderr) == NULL) {
                perror("freopen stderr");
                return -errno;
            }
        }
        else {
            if (freopen(redirection_path, "a+", stdout) == NULL) {
                perror("freopen stdout");
                return -errno;
            }

            if (freopen(redirection_path, "a+", stderr) == NULL) {
                perror("freopen stderr");
                return -errno;
            }
        }

        if (freopen("/dev/null", "r", stdin) == NULL) {
            perror("freopen stdin");
            return -errno;
        }

        return 0;
    }

#define panda_log_with_tag(TAG,fmt,...)\
	do {\
		char buf[25] = "";\
		time_t t = time(NULL);\
		snprintf(buf, sizeof(buf), "%s", ctime(&t));\
		printf("[%5s] ", TAG);\
		printf("[%s] ", buf);\
		printf("[%s] [%s] [%d] ", __FILE__, __FUNCTION__, __LINE__);\
		printf(fmt, ##__VA_ARGS__);\
	} while(0);


#define panda_log_info(fmt,args...) panda_log_with_tag("INFO",fmt,##args)


#define panda_log_warn(fmt,args...) panda_log_with_tag("WARN",fmt,##args)


#define panda_log_trace() panda_log_with_tag("TRACE","trace\n")


#define panda_log_error(fmt,args...)\
	do {\
		panda_log_with_tag("ERROR",fmt,##args);\
		fflush(stdout);\
		perror("reason");\
	} while(0)


#define panda_log_fatal(fmt,args...)\
	do {\
		panda_log_with_tag("FATAL",fmt,##args);\
		fflush(stdout);\
		exit(errno);\
	} while(0)


#define panda_log_flush() fflush(stdout)


#ifdef __cplusplus
}
#endif

#endif	//__PANDA_LOG_H__
