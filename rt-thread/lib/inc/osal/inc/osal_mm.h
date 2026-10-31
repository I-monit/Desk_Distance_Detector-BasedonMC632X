#ifndef _FH_OSAL_MM_H
#define _FH_OSAL_MM_H

/* file is open for reading */
#define FH_OSAL_FMODE_READ		((__force fmode_t)0x1)
/* file is open for writing */
#define FH_OSAL_FMODE_WRITE		((__force fmode_t)0x2)
/* file is seekable */
#define FH_OSAL_FMODE_LSEEK		((__force fmode_t)0x4)
/* file can be accessed using pread */
#define FH_OSAL_FMODE_PREAD		((__force fmode_t)0x8)
/* file can be accessed using pwrite */
#define FH_OSAL_FMODE_PWRITE		((__force fmode_t)0x10)
/* File is opened for execution with sys_execve / sys_uselib */
#define FH_OSAL_FMODE_EXEC		((__force fmode_t)0x20)
/* File is opened with O_NDELAY (only set for block devices) */
#define FH_OSAL_FMODE_NDELAY		((__force fmode_t)0x40)
/* File is opened with O_EXCL (only set for block devices) */
#define FH_OSAL_FMODE_EXCL		((__force fmode_t)0x80)
/* File is opened using open(.., 3, ..) and is writeable only for ioctls
   (specialy hack for floppy.c) */
#define FH_OSAL_FMODE_WRITE_IOCTL	((__force fmode_t)0x100)
/* 32bit hashes as llseek() offset (for directories) */
#define FH_OSAL_FMODE_32BITHASH         ((__force fmode_t)0x200)
/* 64bit hashes as llseek() offset (for directories) */
#define FH_OSAL_FMODE_64BITHASH         ((__force fmode_t)0x400)

#define FH_OSAL_FMODE_NOCMTIME		((__force fmode_t)0x800)

/* Expect random access pattern */
#define FH_OSAL_FMODE_RANDOM		((__force fmode_t)0x1000)

/* File is huge (eg. /dev/kmem): treat loff_t as unsigned */
#define FH_OSAL_FMODE_UNSIGNED_OFFSET	((__force fmode_t)0x2000)

/* File is opened with O_PATH; almost nothing can be done with it */
#define FH_OSAL_FMODE_PATH		((__force fmode_t)0x4000)

/* File needs atomic accesses to f_pos */
#define FH_OSAL_FMODE_ATOMIC_POS	((__force fmode_t)0x8000)
/* Write access to underlying fs */
#define FH_OSAL_FMODE_WRITER		((__force fmode_t)0x10000)
/* Has read method(s) */
#define FH_OSAL_FMODE_CAN_READ          ((__force fmode_t)0x20000)
/* Has write method(s) */
#define FH_OSAL_FMODE_CAN_WRITE         ((__force fmode_t)0x40000)

/* File was opened by fanotify and shouldn't generate fanotify events */
#define FH_OSAL_FMODE_NONOTIFY		((__force fmode_t)0x4000000)


#define FH_OSAL_PROT_READ	0x1		/* page can be read */
#define FH_OSAL_PROT_WRITE	0x2		/* page can be written */
#define FH_OSAL_PROT_EXEC	0x4		/* page can be executed */
#define FH_OSAL_PROT_SEM	0x8		/* page may be used for atomic ops */
#define FH_OSAL_PROT_NONE	0x0		/* page can not be accessed */
#define FH_OSAL_PROT_GROWSDOWN	0x01000000	/* mprotect flag: extend change to start of growsdown vma */
#define FH_OSAL_PROT_GROWSUP	0x02000000	/* mprotect flag: extend change to end of growsup vma */

#define FH_OSAL_MAP_SHARED	0x01		/* Share changes */
#define FH_OSAL_MAP_PRIVATE	0x02		/* Changes are private */
#define FH_OSAL_MAP_TYPE	0x0f		/* Mask for type of mapping (OSF/1 is _wrong_) */
#define FH_OSAL_MAP_FIXED	0x100		/* Interpret addr exactly */
#define FH_OSAL_MAP_ANONYMOUS	0x10		/* don't use a file */

#define FH_OSAL_PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)

extern void * high_memory;

typedef struct fh_osal_vm_area{
    void *vm_area_struct;
}fh_osal_vm_area_t;

unsigned long long fh_osal_find_paddr_in_vma(unsigned long *vaddr, int *cached);
unsigned long fh_osal_vm_mmap_pgoff(void *file, unsigned long addr,
	unsigned long len, unsigned long prot,
	unsigned long flag, unsigned long pgoff);
unsigned long long fh_osal_vma_get_shifted_pgoff(fh_osal_vm_area_t *vma);
int fh_osal_vma_get_vm_range(fh_osal_vm_area_t *vma, unsigned long *vm_start, unsigned long *vm_end);
int fh_osal_remap_pfn_range_vma(fh_osal_vm_area_t *vma, int cached);
int fh_osal_vm_munmap(unsigned long start, size_t len);
phys_addr_t fh_osal_virt_to_phys(const volatile void *x);


#endif
