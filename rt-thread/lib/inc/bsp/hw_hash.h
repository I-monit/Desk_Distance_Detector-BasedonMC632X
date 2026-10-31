
#ifndef __HW_HASH_H__
#define __HW_HASH_H__



#define FH_MHASH_DEVICE_NAME                 "fh_hash"
#define FH_HASH_PLAT_DEVICE_NAME	"fh_hash"

#define FH_HASH_MISC_DEVICE_NAME                 "fh_hash"




#define FH_HASH_IOCTL_MAGIC             'h'
#define FH_HASH_CALC             (1)




struct fh_hash_param_t
{
    unsigned char* buffer;
    unsigned char* res;
    unsigned long len;
    unsigned long shalen;
};




#endif /* __HW_HASH_H__ */
