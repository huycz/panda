#include "panda_procedure.h"
#include "panda_log.h"
#include "st.h"
#include <errno.h>
#include <string.h>
#include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif


#define MAX_PROCEDURE_PER_GROUP sizeof(int32_t)


    static void (*panda_procedure_init)(void *data) = NULL;
    static void (*panda_procedure_exit)(void *data) = NULL;
    static uint32_t request_data_size = DEFAULT_REQUEST_DATA_SIZE;
    static uint32_t reply_data_size = DEFAULT_REPLY_DATA_SIZE;


    typedef struct panda_procedure_group_data_s
    {
        int32_t total;
        int32_t index;
        void *req_data;
        size_t req_count;
        void *rep_data;
        size_t rep_count;
        panda_procedure_t *procedure[MAX_PROCEDURE_PER_GROUP];
        st_thread_t thread[MAX_PROCEDURE_PER_GROUP];
        st_thread_t owner;
        void *private_data;
    } panda_procedure_group_data_t;


    panda_procedure_group_data_t *__panda_procedure_group_data_alloc()
    {
        panda_procedure_group_data_t *pg_data = (panda_procedure_group_data_t *)malloc(sizeof(panda_procedure_group_data_t));
        assert(pg_data != NULL);
        memset(pg_data, 0x00, sizeof(*pg_data));
        pg_data->owner = st_thread_self();
        pg_data->req_data = malloc(request_data_size);
        assert(pg_data->req_data != NULL);
        pg_data->rep_data = malloc(reply_data_size);
        assert(pg_data->rep_data != NULL);

        return pg_data;
    }


    int32_t panda_procedure_group_init(void (*init)(void *data), void (*exit)(void *data))
    {
        assert(init != NULL);
        assert(exit != NULL);
        panda_procedure_init = init;
        panda_procedure_exit = exit;

        st_init();

        return 0;
    }


    void *__panda_procedure_group_start(void *pgdata)
    {
        panda_procedure_group_data_t *pg_data = (panda_procedure_group_data_t *)pgdata;
        assert(pgdata != NULL);
        assert(panda_procedure_init != NULL);
        assert(panda_procedure_exit != NULL);
        panda_procedure_init(pgdata);
        panda_procedure_exit(pgdata);
        //TODO reply here
        panda_log_info("rep : %s\n", (char *)pg_data->rep_data);
        panda_log_flush();
        panda_procedure_group_destroy(pgdata);
        st_thread_exit(NULL);
        return pgdata;
    }


    void *__panda_procedure_start(void *pgdata)
    {
        panda_procedure_group_data_t *pg_data = (panda_procedure_group_data_t *)pgdata;
        assert(pgdata != NULL);
        if (pg_data->procedure[pg_data->index]->type && REQUEST)
            pg_data->procedure[pg_data->index]->request(pg_data->req_data, pg_data->req_count, pg_data);
        if (pg_data->procedure[pg_data->index]->type && REPLY)
            pg_data->procedure[pg_data->index]->reply(pg_data->rep_data, &(pg_data->rep_count), pg_data);

        st_thread_exit(NULL);

        return pgdata;
    }


    int32_t panda_procedure_group_create(void *pgdata)
    {
        st_thread_t st_pg = NULL;
        panda_procedure_group_data_t *pg_data = (panda_procedure_group_data_t *)pgdata;
        assert(pgdata != NULL);

        st_pg = st_thread_create(__panda_procedure_group_start, pg_data, 0, 0);
        assert(st_pg != NULL);

        return 0;
    }


    int32_t panda_procedure_group_destroy(void *pgdata)
    {
        panda_procedure_group_data_t *pg_data = (panda_procedure_group_data_t *)pgdata;
        if (pgdata == NULL)
            return -(errno = EINVAL);

        if (pg_data->req_data != NULL)
            free(pg_data->req_data);

        if (pg_data->rep_data != NULL)
            free(pg_data->rep_data);

        free(pgdata);

        return 0;
    }


    int32_t panda_procedure_register(panda_procedure_t *proc_vec, size_t proc_cnt, void *pgdata)
    {
        size_t i = 0;
        panda_procedure_group_data_t *pg_data = (panda_procedure_group_data_t *)pgdata;

        if (pg_data == NULL)
            return -(errno = EINVAL);

        for (i = 0; i < proc_cnt; i++)
        {
            if ((proc_vec->type & REQUEST) && (proc_vec->request == NULL))
                return -(errno = EINVAL);
            if ((proc_vec->type & REPLY) && (proc_vec->reply == NULL))
                return -(errno = EINVAL);

            pg_data->procedure[pg_data->total++] = proc_vec++;
        }

        return 0;
    }


    int32_t panda_procedure_commit(void *pgdata)
    {
        panda_procedure_group_data_t *pg_data = (panda_procedure_group_data_t *)pgdata;
        assert(pgdata != NULL);

        for (pg_data->index = 0; pg_data->index < pg_data->total; pg_data->index++) {
            pg_data->thread[pg_data->index] = st_thread_create(__panda_procedure_start, pg_data, 1, 0);
        }

        for (pg_data->index = 0; pg_data->index < pg_data->total; pg_data->index++) {
            if (st_thread_join(pg_data->thread[pg_data->index], NULL) < 0)
                panda_log_error("st_thread_join fail\n");
        }

        pg_data->total = 0;
        pg_data->index = 0;

        return 0;
    }


    int32_t panda_procedure_group_data_init(void *pgdata, void *private_data)
    {
        panda_procedure_group_data_t *pg_data = (panda_procedure_group_data_t *)pgdata;
        assert(pgdata != NULL);
        pg_data->private_data = private_data;

        return 0;
    }


    void *panda_procedure_group_data(void *pgdata)
    {
        panda_procedure_group_data_t *pg_data = (panda_procedure_group_data_t *)pgdata;
        assert(pgdata != NULL);
        return pg_data->private_data;
    }


    int32_t panda_procedure_set_data_size(uint32_t req_size, uint32_t rep_size)
    {
        if ((req_size > MAX_REQUEST_REPLY_DATA_SIZE) || (rep_size > MAX_REQUEST_REPLY_DATA_SIZE))
            return -(errno = EINVAL);

        request_data_size = req_size;
        reply_data_size = rep_size;

        return 0;
    }


    char test_req[32] = {"A Hello Panda"};

    int32_t __panda_procedure_request_data_init(void *pgdata)
    {
        panda_procedure_group_data_t *pg_data = (panda_procedure_group_data_t *)pgdata;
        assert(pgdata != NULL);

        pg_data->req_count = strlen(test_req);
        strncpy((char *)pg_data->req_data, test_req, pg_data->req_count);
        panda_log_info("req : %s\n", (char *)pg_data->req_data);
        return strlen((char *)pg_data->req_data);
    }


    void panda_procedure_loop()
    {
        uint32_t i = 0;

        for (i = 0; i < 100; i++)
        {
            panda_procedure_group_data_t * pg_data = __panda_procedure_group_data_alloc();
            //TODO get request data to pg_data->req_data here
            __panda_procedure_request_data_init(pg_data);
            panda_procedure_group_create(pg_data);
            test_req[0] = i % 26 + 'A';
        }

        while (1)
            st_sleep(10000);
    }


#ifdef __cplusplus
}
#endif
