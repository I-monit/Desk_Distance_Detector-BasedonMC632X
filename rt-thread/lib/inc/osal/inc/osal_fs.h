#ifndef _FH_OSAL_PROC_H
#define _FH_OSAL_PROC_H

#define FH_OSAL_POLLIN		0x0001
#define FH_OSAL_POLLPRI		0x0002
#define FH_OSAL_POLLOUT		0x0004
#define FH_OSAL_POLLERR		0x0008
#define FH_OSAL_POLLHUP		0x0010
#define FH_OSAL_POLLNVAL	0x0020

/* The rest seem to be more-or-less nonstandard. Check them! */
#define FH_OSAL_POLLRDNORM	0x0040
#define FH_OSAL_POLLRDBAND	0x0080
#ifndef FH_OSAL_POLLWRNORM
#define FH_OSAL_POLLWRNORM	0x0100
#endif
#ifndef FH_OSAL_POLLWRBAND
#define FH_OSAL_POLLWRBAND	0x0200
#endif
#ifndef FH_OSAL_POLLMSG
#define FH_OSAL_POLLMSG		0x0400
#endif
#ifndef FH_OSAL_POLLREMOVE
#define FH_OSAL_POLLREMOVE	0x1000
#endif
#ifndef FH_OSAL_POLLRDHUP
#define FH_OSAL_POLLRDHUP       0x2000
#endif

#define FH_OSAL_POLLFREE	0x4000	/* currently only for epoll */

#define FH_OSAL_POLL_BUSY_LOOP	0x8000

typedef struct fh_osal_fileops {
	int (*open) (void * inode, void * file);
	ssize_t (*read) (void * file, char __user *, size_t, loff_t *);
	ssize_t (*write) (void * file, const char __user *, size_t, loff_t *);
	loff_t (*llseek) (void * file, loff_t, int);
	int (*release) (void * inode, void * file);
	long (*unlocked_ioctl) (void * file, unsigned int, unsigned long);
	int (*mmap) (void * file, void * vm_area_struct);
	unsigned int (*poll) (void * file, void * poll_table_struct);
}fh_osal_fileops_t;

typedef struct fh_osal_proccontext {
	void *fileops;
	void *proc_dir_entry;
}fh_osal_proccontext_t;

fh_osal_proccontext_t *fh_osal_proccreate(char* name, fh_osal_fileops_t *fops, mode_t mode);
void fh_osal_procremove(fh_osal_proccontext_t *context);
void *fh_osal_get_file_private_data(void *file);
void fh_osal_set_file_private_data(void *file, void *data);
fmode_t fh_osal_get_file_f_mode(void *file);
void fh_osal_set_file_f_mode(void *file, fmode_t mode);
unsigned char* fh_osal_get_file_d_iname(void *file);



void fh_osal_poll_wait(void *file, fh_osal_wait_t * wait, void *p);
bool fh_osal_poll_does_not_wait(const void *p);
unsigned long fh_osal_poll_requested_events(const void *p);
int fh_osal_vfs_stat_size(char *path, loff_t *size);
int fh_osal_kernel_read_file_from_path(char *path, void *buf, loff_t *size, loff_t max_size);

#endif

// typedef struct fh_osal_fileops{
// 	int (*open) (struct inode *, struct file *);
// 	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
// 	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
// 	loff_t (*llseek) (struct file *, loff_t, int);
// 	int (*release) (struct inode *, struct file *);
// 	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
// 	int (*mmap) (struct file *, struct vm_area_struct *);
// }fh_osal_fileops_t;
