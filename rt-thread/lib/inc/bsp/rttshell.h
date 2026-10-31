#ifndef __RTT_SHELL_H__
#define __RTT_SHELL_H__

typedef long (*shell_func)(void);

struct shell_table_entry
{
    const char *name;
    const char *desc;
    shell_func  func;
};

#define DATA_SEC(name) __attribute__((section(name)))

#define SHELL_CMD_EXPORT(name, desc)                \
            const char __fsym___cmd_##name##_name[] DATA_SEC(".rodata.name") = "__cmd_"#name; \
            const char __fsym___cmd_##name##_desc[] DATA_SEC(".rodata.name") = #desc;         \
            const struct shell_table_entry __fsym_##name DATA_SEC("FSymTab") =                \
            {                                       \
                __fsym___cmd_##name##_name,         \
                __fsym___cmd_##name##_desc,         \
                (shell_func)&name                   \
            };


#endif
