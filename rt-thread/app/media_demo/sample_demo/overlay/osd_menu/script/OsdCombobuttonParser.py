#!/usr/bin/env python3
# -*- coding:utf-8 -*-
""" code generatation of combobutton object """

from common import *
import OsdObjParser


def get_combobutton_max_num(ini, obj):
    """
    ## 返回combobutton选项数量字符串
    - **ini** configparser对象
    - **obj** combobutton对象名
    """
    text = ini.get(obj, 'text')
    lang = str(text).split('##')
    string = lang[0].split(';')
    return str(len(string))


class OsdCombobuttonParser:
    """
    ## osd combobutton object parser
    - **ini** configparser对象
    - **obj** combobutton对象名
    """

    def __init__(self, ini, obj):
        self.obj = obj
        self.buttontype = ''
        self.jumpto = []
        self.max_num = -1
        self.val_pos = -1
        self.out_str = ''
        self.ini = ini
        self.oop = OsdObjParser.OsdObjParser(ini, obj)
        self.obj_name = get_obj_name(obj, 'combobutton')

    def get_val_pos(self, obj):
        """ get attribute val_pos """
        if 'val_pos' in self.ini.options(obj):
            self.val_pos = self.ini.get(obj, 'val_pos')

    def get_buttontype(self, obj):
        """ get attribute buttontype """
        if 'buttontype' in self.ini.options(obj):
            self.buttontype = self.ini.get(obj, 'buttontype').replace(';',',')

    def get_jumpto(self, obj):
        """ get attribute jumpto """
        if 'jumpto' in self.ini.options(obj):
            self.jumpto = self.ini.get(obj, 'jumpto').split(';')

    def get_max_num(self):
        """ get attribute max_num """
        self.max_num = 'NUM_%s - 1' % self.obj_name

    def gen_code_common(self):
        """ print code common """
        return '    combobutton_t *self = &%s;\n\
    menu_conf_t *param = &defaultConf;\n\
    combobutton_configure(self, param);\n' % self.obj_name

    def gen_code_val_pos(self):
        """ print code val_pos """
        if self.val_pos == -1:
            return ''
        else:
            return '    combobutton_setValuePos(self, %s);\n' % self.val_pos

    def gen_code_jumpto(self):
        """ print code jumpto """
        extern = ''
        atrribute = '    osd_obj_t *test_atrribute_button_jumpto[%s] = {' %self.max_num
        for i in self.jumpto:
            if i != '0':
                extern += ('    extern menu_t menu' + camel_case(i) + ';\n')
                atrribute += ('&menu' + camel_case(i) + '.base' + ', ')
            else:
                atrribute += (i + ', ')
        atrribute = atrribute.rstrip(', ')
        max_num = '    int max_num = %s;\n' %self.max_num
        setArray = '    combobutton_setArray(self, test_atrribute_button_type, test_atrribute_button_jumpto, test_atrribute_button ,max_num);\n'
        jumpto = max_num + extern + atrribute + '};\n' + setArray
        return jumpto


    def gen_code_combobuttontype(self):
        """ print code combobuttontype """

        return '    int test_atrribute_button_type[%s] = {%s};\n' % (self.max_num, self.buttontype)

    def gen_code_max_num(self):
        """ print code max_num """
        if self.max_num == -1:
            return ''
        else:
            return '    combobutton_setMaxNum(self, %s);\n' % self.max_num

    def gen_code_head(self):
        """ print code head """
        return 'atrribute_button_t test_atrribute_button[%s];\n\
static void %s_init(void) {\n' %(self.max_num, self.obj_name)

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
        self.get_buttontype(self.obj)
        self.get_jumpto(self.obj)

        self.out_str = self.out_str + self.gen_code_head()
        self.out_str = self.out_str + self.gen_code_common()
        self.out_str = self.out_str + self.gen_code_combobuttontype()
        self.out_str = self.out_str + self.gen_code_jumpto()
        self.out_str = self.out_str + self.gen_code_max_num()
        self.out_str = self.out_str + self.gen_code_val_pos()
        self.out_str = self.out_str + self.oop.run()
        self.out_str = self.out_str + self.gen_code_tail()
        # print(self.out_str)

        return self.out_str
