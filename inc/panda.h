#ifndef _PANDA_H_
#define _PANDA_H_

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum {
		ONE_SHORT,
		FLAT_STREAM,
		RT_MESSAGE,
	} panda_work_mode;

	typedef enum {
        PROT_NONE = 0x00,
		PROT_TCP  = 0x01,
		PROT_UDP  = 0x02,
	} panda_service_prot_flag;

	typedef struct {
		panda_work_mode         mode; /* work mode */
		panda_service_prot_flag prot; /* service protocal */
		unsigned short          port; /* service port */
		unsigned short          id;   /* service type */
	} panda_attribute;

    typedef int (*panda_init_func_t)(panda_attribute*);
    typedef void (*panda_destroy_func_t)(int);
    typedef int (*panda_request_func_t)(const void *, int, const void*, int, int,
                                int (*)(const void *, int, const void*, int));
    typedef int (*panda_alloc_func_t)();
    typedef int (*panda_free_func_t)(int);

	/*
	 * init all resource and fill register info
	 * @ attr       fill the attribute for register monitor
	 * @ return     zero success, less zero failure
	 */
	int panda_init(panda_attribute *attr);
	/*
	 * destroy all resource
	 * @ timeout    if any request or reply still work, waiting some time
	 * @ return     don't care any error
	 */
	void panda_destroy(int timeout);
	/*
	 * request panda service
     * @ panda       panda instance
	 * @ buf         request message
	 * @ size        request message size
     * @ flag        request flag
	 * @ reply       reply callback, must non-NULL function, please!
     * @ return      zero is success, -1 mean eof
	 */
	int panda_request(const void *obj, int panda, const void *buf, int size, int flag,
         int (*reply)(const void *obj, int panda, const void *buf, int size));
    /*
     * alloc panda instance when client connect
     * @ return       panda identify, less zero failure
     */
    int panda_alloc();
    /*
     * free panda instance when client disconnect 
     * @ panda        panda instance
     * @ return       zero success, less zero failure
     */
    int panda_free(int panda);

#ifdef __cplusplus
}
#endif

#endif /* _PANDA_H_ */
