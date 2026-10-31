#!/usr/bin/env python3
# -*- coding:utf-8 -*-
""" code generatation of combobox object """

from common import *
import OsdObjParser


def get_combobox_max_num(ini, obj):
    """
    ## 返回combobox选项数量字符串
    - **ini** configparser对象
    - **obj** combobox对象名
    """
    text = ini.get(obj, 'text')
    lang = str(text).split('##')
    string = lang[0].split(';')
    return str(len(string))


class OsdComboboxParser:
    """
    ## osd combobox object parser
    - **ini** configparser对象
    - **obj** combobox对象名
    """

    def __init__(self, ini, obj):
        self.obj = obj
        self.max_num = -1
        self.val_pos = -1
        self.out_str = ''
        self.ini = ini
        self.oop = OsdObjParser.OsdObjParser(ini, obj)
        self.obj_name = get_obj_name(obj, 'combobox')

    def get_val_pos(self, obj):
        """ get attribute val_pos """
        if 'val_pos' in self.ini.options(obj):
            self.val_pos = self.ini.get(obj, 'val_pos')

    def get_max_num(self):
        """ get attribute max_num """
        self.max_num = 'NUM_%s - 1' % self.obj_name

    def gen_code_common(self):
        """ print code common """
        return '    combobox_t *self = &%s;\n\
    menu_conf_t *param = &defaultConf;\n\
    combobox_configure(self, param);\n' % self.obj_name

    def gen_code_val_pos(self):
        """ print code val_pos """
        if self.val_pos == -1:
            return ''
        else:
            return '    combobox_setValuePos(self, %s);\n' % self.val_pos

    def gen_code_max_num(self):
        """ print code max_num """
        if self.max_num == -1:
            return ''
        else:
            return '    combobox_setMaxNum(self, %s);\n' % self.max_num

    def gen_code_head(self):
        """ print code head """
        return 'static void %s_init(void) {\n' % self.obj_name

    def gen_code_tail(self):
        """ print code tail """
        return '}\n\n'

    def run(self):
        """
        # 创建menu对象初始化代码
        - return 生成代码字符串
        """
        self.get_max_num()
        self.get_val_pos(self.obj)

        self.out_str = self.out_str + self.gen_code_head()
        self.out_str = self.out_str + self.gen_code_common()
        self.out_str = self.out_str + self.gen_code_max_num()
        self.out_str = self.out_str + self.gen_code_val_pos()
        self.out_str = self.out_str + self.oop.run()
        self.out_str = self.out_str + self.gen_code_tail()
        # print(self.out_str)

        return self.out_str
