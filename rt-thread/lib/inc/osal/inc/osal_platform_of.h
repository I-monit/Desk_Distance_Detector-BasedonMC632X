#ifndef _FH_OSAL_PLATFORM_OF_H
#define _FH_OSAL_PLATFORM_OF_H

int fh_osal_platform_get_modparam_uint(void *pdev, const char *name, unsigned int *value);
int fh_osal_platform_get_modparam_string(void *pdev, const char *name, unsigned int size, char *value);
void fh_osal_platform_set_drvdata(void *pdev, void *data);
void *fh_osal_platform_get_drvdata(void *pdev);
void *fh_osal_of_match_device(void* matches, void* dev);
int fh_osal_of_alias_get_id(void *np, const char *stem);
int fh_osal_platform_driver_register(void *drv);
void fh_osal_platform_driver_unregister(void *drv);
void* fh_osal_platform_get_resource_byname(void *dev, unsigned int type,
	const char *name);
void* fh_osal_platform_get_resource(void *dev, unsigned int type,
	unsigned int num);
int fh_osal_platform_get_irq(void *dev, unsigned int num);
int fh_osal_platform_get_irq_byname(void *dev, const char *name);

#endif