#ifndef __FH__TIME_STAT_H__
#define __FH__TIME_STAT_H__
#include <rtdef.h>

#define LIST_CLASS_NUM_MAX 4
#define TIME_MAX_NAME_LEN 40

struct fbv_time_stat_class
{
    rt_list_t time_node_list;

    char name[TIME_MAX_NAME_LEN];
    rt_uint32_t num;
};

#define time_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = (pos)->next; (pos) != head; pos = n, n = (pos)->next)

void fbv_time_stat_init(char *class_name);
void fbv_time_stat_add(char *class_name, char *name);
void fbv_time_stat_prinf(char *class_name);

#endif
