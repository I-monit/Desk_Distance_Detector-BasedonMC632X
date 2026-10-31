#include <linux/kernel.h>
#include "src/nodelist.h"

#include "porting.h"
#include "sys/time.h"
#include "asm/page.h"

time_t jffs2_get_timestamp(void)
{
    struct timeval tval;

    gettimeofday(&tval, RT_NULL);
    return tval.tv_sec;
}

void jffs2_get_info_from_sb(void * data, struct jffs2_fs_info * info)
{
    struct jffs2_fs_info;
    struct super_block *jffs2_sb;
    struct jffs2_sb_info *c;

    jffs2_sb = (struct super_block *)(data);
    c = JFFS2_SB_INFO(jffs2_sb);

    info->sector_size = c->sector_size;
    info->nr_blocks = c->nr_blocks;
    info->free_size = c->free_size + c->dirty_size; /*fixme need test!*/
    if (info->free_size > c->sector_size * c->resv_blocks_write)
        info->free_size -= c->sector_size * c->resv_blocks_write;
    else
        info->free_size = 0;

    info->f_bfree = info->free_size >> PAGE_SHIFT;
    info->f_bsize = 1 << PAGE_SHIFT;
    info->f_blocks = c->flash_size >> PAGE_SHIFT;
}

int jffs2_porting_stat(cyg_mtab_entry * mte, cyg_dir dir, const char *name,
		      void * stat_buf)
{
	return jffs2_fste.stat(mte, mte->root, name, (struct stat *)stat_buf);	
}
