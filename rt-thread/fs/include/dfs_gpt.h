/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DFS_GPT_H__
#define __DFS_GPT_H__

#include <dfs.h>
#include <dfs_fs.h>

#ifdef __cplusplus
extern "C" {
#endif

int check_gpt_label(void *mbuffer, void *gpt_header);
void get_gpt_partition_info(uint32_t *first_part_lba, uint32_t *entry_len, uint32_t *parts);
int dfs_filesystem_get_gpt_partition(struct dfs_partition *part, uint8_t *buf, uint32_t pindex);

#ifdef __cplusplus
}
#endif

#endif
