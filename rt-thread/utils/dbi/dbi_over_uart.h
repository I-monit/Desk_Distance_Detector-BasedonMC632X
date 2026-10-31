#ifndef __DBI_OVER_UART_H__
#define __DBI_OVER_UART_H__

struct dbi_uart_config
{
    int index;   /* 0——uart0; 1——uart1; 2——uart2; 3——uart3; 4——cdc-vcom*/
    int *cancel;
};

int *uart_dbi_thread(void *param);

#endif
