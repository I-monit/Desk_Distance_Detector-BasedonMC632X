
#include "rtthread.h"
#include "port_rt.h"
#include <string.h>
#include <ubiport_common.h>
#ifdef USE_SCHEDULE_WORK
static rt_mq_t gpMsgq = NULL;
static rt_uint8_t gMsg_pool[512];

ALIGN(RT_ALIGN_SIZE)

#define MSG_MAXSIZE 8
void ubi_schedule_thread(void *param)
{
    char buf[MSG_MAXSIZE] = {0};
    while (1)
    {
        memset(buf, 0, sizeof(buf));
        if (rt_mq_recv(gpMsgq, &buf, sizeof(void *), RT_WAITING_FOREVER) == RT_EOK)
        {
/*            rt_kprintf("manage_thread: recv msg from msg queue, the content:%c\n", buf);*/
            struct work_struct **pWork = (struct work_struct **)buf;

            rt_kprintf("pwork:%p\n", *pWork);
            (*pWork)->func(*pWork);
        }
    }
    rt_kprintf("manage_thread: detach mq \n");
    rt_mq_detach(gpMsgq);
    rt_free(gpMsgq);
}

void ubi_InitSchedule(void)
{
    if (gpMsgq != NULL)
    {
        return;
    }
    rt_err_t result;
    rt_thread_t thread;
    gpMsgq = rt_malloc(sizeof(struct rt_messagequeue));
    result = rt_mq_init(gpMsgq,
                        "schmq",
                        &gMsg_pool[0],     /* �ڴ��ָ�� msg_pool */
                        1,                 /* ÿ����Ϣ�Ĵ�С�� 1 �ֽ� */
                        sizeof(gMsg_pool), /* �ڴ�صĴ�С�� msg_pool �Ĵ�С */
                        RT_IPC_FLAG_FIFO); /* ����ж���̵߳ȴ������������ȵõ��ķ���������Ϣ */

    if (result != RT_EOK)
    {
        rt_kprintf("init message queue failed.\n");
        return;
    }
    thread = rt_thread_create("ubi_schedule", ubi_schedule_thread, RT_NULL,
                              2 * 1024, RT_THREAD_PRIORITY_MAX / 3 - 1, 20);
    if (thread != RT_NULL)
        rt_thread_startup(thread);
}
void schedule_work(struct work_struct *pWork)
{
    ubiPort_Log("schedule_work,%p,%p\n", pWork, &pWork);
    if (gpMsgq == NULL)
    {
        rt_kprintf("gpMsgq NULL\n");
        return;
    }
    int result = rt_mq_send(gpMsgq, (void *)&pWork, sizeof(pWork));
    if (result != RT_EOK)
    {
        rt_kprintf("rt_mq_send ERR\n");
    }
}
#endif
