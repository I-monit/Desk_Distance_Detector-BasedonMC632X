# OSD菜单生成方法说明

## 生成过程

单个字库没有超过，不需要分割字库的情况

- 按格式完成 `in/xxx.ini` (type,name必填，其它的选填,若不填，则为默认值，默认值参考configure.c)
- 运行 `OriginGen.py`，运行 `run.sh`
- 将script/user文件夹下的 `user_string.c`， `user_menu.c`， `user_menu.h` 复制到 OSD/mod中，将script/menu文件夹下的文件复制到OSD/mod/menu中。
