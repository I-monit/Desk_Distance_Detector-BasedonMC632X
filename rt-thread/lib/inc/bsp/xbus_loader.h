#ifndef __xbus_loader_h__
#define __xbus_loader_h__

enum 
{
    XBUS_FW_LOAD_POLICY_TRY    = 1,
    XBUS_FW_LOAD_POLICY_ALWAYS = 2,
};

typedef int firmware_read_func(unsigned int address);

extern int xbus_loader_init(unsigned int policy, /*XBUS_FW_LOAD_POLICY_XXX*/
	unsigned int fa,  /*firmware load address*/
	firmware_read_func *read, /*firmware read callback*/
	unsigned int rx_thread_priority,  /*priority of rx thread*/
	unsigned int timeout); /*timeout in seconds*/


#endif /*__xbus_loader_h__*/

