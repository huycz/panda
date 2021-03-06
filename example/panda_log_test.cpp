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


#include "panda_log.h"

int main(int argc, char *argv[])
{
    char log_path[] = "./log";

    //will show in terminal
    panda_log_info("Hello Panda\n");
    panda_log_warn("Hello Panda\n");
    panda_log_error("Hello Panda\n");
    panda_log_trace();


    //will show in log
    panda_log_init(log_path);
    panda_log_info("Hello Panda\n");
    panda_log_warn("Hello Panda\n");
    panda_log_error("Hello Panda\n");
    panda_log_trace();

    return 0;
}
