#ifndef __PANDA_PROCEDURE_H__
#define __PANDA_PROCEDURE_H__


/*
 * panda_procedure.h : one service contains a group of procedures
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


#include <stdint.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif


#define MAX_REQUEST_REPLY_DATA_SIZE (1024*1024*128)
#define DEFAULT_REQUEST_DATA_SIZE 8192
#define DEFAULT_REPLY_DATA_SIZE 8192

    typedef enum panda_procedure_type_e
    {
        REQUEST = 0x1,
        REPLY   = 0x2,
    }
    panda_procedure_type_t;

    typedef struct panda_procedure_group_data_s panda_procedure_group_data_t;

    typedef struct panda_procedure_s
    {
        uint32_t type;
        int32_t (*request) (void *req_data, size_t count, panda_procedure_group_data_t *pgdata);
        int32_t (*reply) (void *rep_data, size_t *count, panda_procedure_group_data_t *pgdata);
    } panda_procedure_t;

    int32_t panda_procedure_register(panda_procedure_t *pro_vec, size_t pro_cnt, panda_procedure_group_data_t *pgdata);

    int32_t panda_procedure_commit(panda_procedure_group_data_t *pgdata);

    int32_t panda_procedure_group_init(void (*init)(panda_procedure_group_data_t *data), void (*exit)(panda_procedure_group_data_t *data));

    void *panda_procedure_group_data(panda_procedure_group_data_t *pgdata);

    int32_t panda_procedure_group_data_init(panda_procedure_group_data_t *pgdata, void *private_data);

    int32_t panda_procedure_group_set_data_size(uint32_t req_size, uint32_t rep_size);

    int32_t panda_procedure_group_create(panda_procedure_group_data_t *pgdata);

    int32_t panda_procedure_group_destroy(panda_procedure_group_data_t *pgdata);

    void panda_procedure_loop();


#ifdef __cplusplus
}
#endif


#endif	//__PANDA_PROCEDURE_H__
