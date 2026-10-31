#!/usr/bin/env python3
# -*- coding:utf-8 -*-
""" code generatation of menu object """

from common import *
import OsdObjParser


class OsdMenuParser:
    """
    ## osd menu object parser
    - **ini** configparser对象
    - **obj** menu对象名
    """

    def __init__(self, ini, obj):
        self.obj = obj
        self.menuid = ''
        self.autoorder = -1
        self.out_str = ''
        self.ini = ini
        self.oop = OsdObjParser.OsdObjParser(ini, obj)
        self.obj_name = get_obj_name(obj, 'menu')

    def get_autoorder(self, obj):
        """ get attribute autoorder """
        if 'autoorder' in self.ini.options(obj):
            self.autoorder = self.ini.get(obj, 'autoorder')

    def get_menuid(self, obj):
        """ get attribute menuid """
        self.menuid = 'MENU_' + obj.upper()

    def gen_code_common(self):
        """ print code common """
        return '    menu_t *self = &%s;\n\
    menu_conf_t *param = &defaultConf;\n\
    menu_configure(self, param);\n' % self.obj_name

    def gen_code_menuid(self):
        """ print code menuid """
        return '    menu_setId(self, %s);\n' % self.menuid

    def gen_code_autoorder(self):
        """ print code autoorder """
        if self.autoorder == -1:
            return ''
        else:
            return '    menu_setAutoOrder(self, %s);\n' % self.autoorder

    def gen_code_head(self):
        """ print code head """
        return 'static void %s_init(void) {\n' % self.obj_name

    def gen_code_tail(self):
        """ print code tail """
        return '    \n}\n\n'

    def run(self):
        """
        # 创建menu对象初始化代码
        - return 生成代码字符串
        """
        self.get_menuid(self.obj)
        self.get_autoorder(self.obj)

        self.out_str = self.out_str + self.gen_code_head()
        self.out_str = self.out_str + self.gen_code_common()
        self.out_str = self.out_str + self.gen_code_menuid()
        self.out_str = self.out_str + self.gen_code_autoorder()
        self.out_str = self.out_str + self.oop.run()
        self.out_str = self.out_str + self.gen_code_tail()
        # print(self.out_str)

        return self.out_str
