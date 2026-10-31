#include <rtthread.h>
#include "fh_time_stat.h"

struct time_node
{
    rt_list_t list;

    char name[TIME_MAX_NAME_LEN];
    rt_uint32_t time;
};

struct fbv_time_stat_class *g_time_stat_class[LIST_CLASS_NUM_MAX];
static int g_list_num;

static void reverse_list(char *class_name)
{
    rt_list_t *head = RT_NULL;
    int idx = 0;
    int class_id = 0;
    for (idx = 0; idx < g_list_num; idx++)
    {
        if (!rt_strcmp(g_time_stat_class[idx]->name, class_name))
        {
            head = &g_time_stat_class[idx]->time_node_list;
            class_id = idx;
            break;
        }
    }

    if (head == RT_NULL)
    {
        rt_kprintf("class name mismatch!!\n");
        return;
    }

    rt_list_t *temp;
    do
    {
        temp = head->next;
        head->next = head->prev;
        head->prev = temp;
        head = temp;
    } while (head != &g_time_stat_class[class_id]->time_node_list);
}

void fbv_time_stat_init(char *class_name)
{

    if (g_list_num >= LIST_CLASS_NUM_MAX)
    {
        rt_kprintf("time statistics list overflow!!!\n");
    }

    g_time_stat_class[g_list_num] = rt_malloc(sizeof(struct time_node));
    rt_list_init(&g_time_stat_class[g_list_num]->time_node_list);
    rt_strncpy(g_time_stat_class[g_list_num]->name, class_name, TIME_MAX_NAME_LEN);
    g_list_num++;

    return;
}

extern unsigned int read_pts(void);
void fbv_time_stat_add(char *class_name, char *name)
{
    rt_list_t *head = RT_NULL;
    int idx = 0;
    for (idx = 0; idx < g_list_num; idx++)
    {
        if (!rt_strcmp(g_time_stat_class[idx]->name, class_name))
        {
            head = &g_time_stat_class[idx]->time_node_list;
            break;
        }
    }

    if (head == RT_NULL)
    {
        rt_kprintf("class name mismatch!!\n");
        return;
    }

    struct time_node *node;

    node = (struct time_node *)rt_malloc(sizeof(struct time_node));
    if (!node)
    {
        return;
    }
    node->time = read_pts();

    rt_strncpy(node->name, name, TIME_MAX_NAME_LEN);

    rt_list_insert_after(head, &node->list);
}

extern unsigned int get_os_startup_time(void);

void fbv_time_stat_prinf(char *class_name)
{
    rt_list_t *head = RT_NULL;
    rt_list_t *pos = RT_NULL;
    rt_list_t *n = RT_NULL;
    struct time_node *node;
    int idx = 0;
    for (idx = 0; idx < g_list_num; idx++)
    {
        if (!rt_strcmp(g_time_stat_class[idx]->name, class_name))
        {
            head = &g_time_stat_class[idx]->time_node_list;
            break;
        }
    }
    if (head == RT_NULL)
    {
        rt_kprintf("class name mismatch!!\n");
        return;
    }
    unsigned int arc_os_enter_time = get_os_startup_time();
    reverse_list(class_name);
    rt_kprintf("\n***********************%s: Time Stat(pts)*******************************\n", class_name);
    rt_kprintf("   Enter RTT OS @%u\n", arc_os_enter_time);
    time_list_for_each_safe(pos, n, head)
    {
        node = (struct time_node *)pos;
        rt_kprintf("   %s @%d\n", node->name, node->time);
        rt_list_remove(pos);
        rt_free(pos);
    }
    rt_kprintf("****************************%s: End Time Stat****************************\n", class_name);
}
