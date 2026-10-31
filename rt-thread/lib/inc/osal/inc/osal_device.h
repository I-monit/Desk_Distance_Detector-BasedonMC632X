#ifndef _FH_OSAL_DEVICE_H
#define _FH_OSAL_DEVICE_H

#define FH_OSAL_MISC_DYNAMIC_MINOR 255

typedef struct fh_osal_dev {
	char name[48];
	void *dev;
	int minor;
	fh_osal_fileops_t *fops;
	void *sys_fops;
	void *misc_dev;
}fh_osal_dev_t;

int fh_osal_misc_registerdevice(fh_osal_dev_t *fh_osal_dev);
int fh_osal_misc_deregisterdevice(fh_osal_dev_t *fh_osal_dev);
void *fh_osal_get_device(void *dev);
int fh_osal_put_device(void *dev);

#endif