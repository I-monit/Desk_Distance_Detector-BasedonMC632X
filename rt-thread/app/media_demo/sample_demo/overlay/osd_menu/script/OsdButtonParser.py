#!/usr/bin/env python3
# -*- coding:utf-8 -*-
""" code generatation of button object """

from common import *
import OsdObjParser


class OsdButtonParser:
    """
    ## osd button object parser
    - **ini** configparser对象
    - **obj** button对象名
    """

    def __init__(self, ini, obj):
        self.obj = obj
        self.buttontype = -1
        self.val_pos = -1
        self.jumpto = '#unset'
        self.out_str = ''
        self.ini = ini
        self.oop = OsdObjParser.OsdObjParser(ini, obj)
        self.obj_name = get_obj_name(obj, 'button')

    def get_buttontype(self, obj):
        """ get attribute buttontype """
        if 'buttontype' in self.ini.options(obj):
            buttontype = self.ini.get(obj, 'buttontype')
            if buttontype == '0':
                self.buttontype = 'TO_SUBMENU'
            elif buttontype == '1':
                self.buttontype = 'NORMAL'
            else:
                self.buttontype = 'UNKNOWN'

    def get_val_pos(self, obj):
        """ get attribute val_pos """
        if 'val_pos' in self.ini.options(obj):
            self.val_pos = self.ini.get(obj, 'val_pos')

    def get_jumpto(self, obj):
        """ get attribute jumpto """
        if 'jumpto' in self.ini.options(obj):
            self.jumpto = self.ini.get(obj, 'jumpto')

    def gen_code_common(self):
        """ print code common """
        return '    button_t *self = &%s;\n\
    menu_conf_t *param = &defaultConf;\n\
    button_configure(self, param);\n' % self.obj_name

    def gen_code_buttontype(self):
        """ print code buttontype """
        if self.buttontype == -1:
            return ''
        else:
            return '    button_setType(self, %s);\n' % self.buttontype

    def gen_code_val_pos(self):
        """ print code val_pos """
        if self.val_pos == -1:
            return ''
        else:
            return '    button_setValuePos(self, %s);\n' % self.val_pos

    def gen_code_jumpto(self):
        """ print code jumpto """
        if self.jumpto == '#unset':
            return ''
        else:
            return '    extern menu_t <JUMPTO>;\n\
    button_setJumpTo(self, &<JUMPTO>.base);\n'\
    .replace('<JUMPTO>', get_obj_name(self.jumpto, 'menu'))

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
        self.get_buttontype(self.obj)
        self.get_val_pos(self.obj)
        self.get_jumpto(self.obj)

        self.out_str = self.out_str + self.gen_code_head()
        self.out_str = self.out_str + self.gen_code_common()
        self.out_str = self.out_str + self.gen_code_jumpto()
        self.out_str = self.out_str + self.gen_code_buttontype()
        self.out_str = self.out_str + self.gen_code_val_pos()
        self.out_str = self.out_str + self.oop.run()
        self.out_str = self.out_str + self.gen_code_tail()
        # print(self.out_str)

        return self.out_str
