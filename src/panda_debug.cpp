/*
 * panda_debug.cpp : debug methods
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


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <execinfo.h>


#ifdef __cplusplus
extern "C" {
#endif


    void panda_show_maps()
    {
        FILE *fp = fopen("/proc/self/maps", "r");
        char buf[256] = "";

        if (!fp)
            return;

        rewind(fp);

        while (fgets(buf, sizeof(buf), fp)) {
            printf("%s", buf);
        }

        fflush(fp);
        fclose(fp);
    }


    void panda_show_backtrace()
    {
        int j, nptrs;
        void *buffer[32];
        char **strings;

        nptrs = backtrace(buffer, 32);
        strings = backtrace_symbols(buffer, nptrs);
        if (strings == NULL) {
            perror("backtrace_symbols");
            return;
        }

        printf("-----------------------------------------------------------\n");
        for (j = 0; j < nptrs; j++)
            printf("%s\n", strings[j]);
        printf("-----------------------------------------------------------\n");

        free(strings);
    }


#ifdef __cplusplus
}
#endif


