#!/usr/bin/env python3
# -*- coding:utf-8 -*-
""" code generatation of textbox object """

from common import *
import OsdObjParser

class OsdTextboxParser:
    """
    ## osd Twxtbox object parser
    - **ini** configparser对象
    - **obj** text对象名
    """

    def __init__(self,ini,obj):
        self.obj = obj
        self.mode = -1
        self.out_str = ''
        self.ini = ini
        self.oop = OsdObjParser.OsdObjParser(ini, obj)
        self.obj_name = get_obj_name(obj, 'textbox')
    
    def get_mode(self,obj):
        """ get attribute mode """
        if 'mode' in self.ini.options(obj):
            mode=self.ini.get(obj, 'mode')
            if mode =='1':
                self.mode = 'TEXT_CHANGE'
            elif mode == '0':
                self.mode = 'TEXT_DEFAULT'
            else:
                self.mode = 'UNKNOWN'

    def gen_code_common(self):
        """ print code common """
        return '    textbox_t *self = &%s;\n\
    menu_conf_t *param = &defaultConf;\n\
    textbox_configure(self, param);\n' % self.obj_name

    def gen_code_mode(self):
        """ print code mode """
        if self.mode == -1:
            return ''
        else:
            return '    textbox_setMode(self, %s);\n' % self.mode

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
        self.get_mode(self.obj)

        self.out_str = self.out_str + self.gen_code_head()
        self.out_str = self.out_str + self.gen_code_common()
        self.out_str = self.out_str + self.gen_code_mode()
        self.out_str = self.out_str + self.oop.run()
        self.out_str = self.out_str + self.gen_code_tail()

        return self.out_str