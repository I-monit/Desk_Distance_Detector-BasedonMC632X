#!/usr/bin/env python3
# -*- coding:utf-8 -*-
""" code generatation of scrollbar object """

from common import *
import OsdObjParser


class OsdScrollbarParser:
    """
    ## osd scrollbar object parser
    - **ini** configparser对象
    - **obj** scrollbar对象名
    """

    def __init__(self, ini, obj):
        self.obj = obj
        self.min = -1
        self.max = -1
        self.step = -1
        self.val_pos = -1
        self.out_str = ''
        self.ini = ini
        self.oop = OsdObjParser.OsdObjParser(ini, obj)
        self.obj_name = get_obj_name(obj, 'scrollbar')

    def get_val_pos(self, obj):
        """ get attribute val_pos """
        if 'val_pos' in self.ini.options(obj):
            self.val_pos = self.ini.get(obj, 'val_pos')

    def get_min(self, obj):
        """ get attribute min """
        if 'min' in self.ini.options(obj):
            self.min = self.ini.get(obj, 'min')

    def get_max(self, obj):
        """ get attribute max """
        if 'max' in self.ini.options(obj):
            self.max = self.ini.get(obj, 'max')

    def get_step(self, obj):
        """ get attribute step """
        if 'step' in self.ini.options(obj):
            self.step = self.ini.get(obj, 'step')

    def gen_code_common(self):
        """ print code common """
        return '    scrollbar_t *self = &%s;\n\
    menu_conf_t *param = &defaultConf;\n\
    scrollbar_configure(self, param);\n' % self.obj_name

    def gen_code_min(self):
        """ print code min """
        if self.min == -1:
            return ''
        else:
            return '    scrollbar_setMin(self, %s);\n' % self.min

    def gen_code_max(self):
        """ print code max """
        if self.max == -1:
            return ''
        else:
            return '    scrollbar_setMax(self, %s);\n' % self.max

    def gen_code_step(self):
        """ print code step """
        if self.step == -1:
            return ''
        else:
            return '    scrollbar_setStep(self, %s);\n' % self.step

    def gen_code_val_pos(self):
        """ print code val_pos """
        if self.val_pos == -1:
            return ''
        else:
            return '    scrollbar_setValuePos(self, %s);\n' % self.val_pos

    def gen_code_head(self):
        """ print code head """
        return 'static void %s_init(void) {\n' % self.obj_name

    def gen_code_tail(self):
        """ print code tail """
        return '}\n\n'

    def run(self):
        """
        # 创建 scrollbar 对象初始化代码
        - return 生成代码字符串
        """
        self.get_min(self.obj)
        self.get_max(self.obj)
        self.get_step(self.obj)
        self.get_val_pos(self.obj)

        self.out_str = self.out_str + self.gen_code_head()
        self.out_str = self.out_str + self.gen_code_common()
        self.out_str = self.out_str + self.gen_code_min()
        self.out_str = self.out_str + self.gen_code_max()
        self.out_str = self.out_str + self.gen_code_step()
        self.out_str = self.out_str + self.gen_code_val_pos()
        self.out_str = self.out_str + self.oop.run()
        self.out_str = self.out_str + self.gen_code_tail()
        # print(self.out_str)

        return self.out_str
