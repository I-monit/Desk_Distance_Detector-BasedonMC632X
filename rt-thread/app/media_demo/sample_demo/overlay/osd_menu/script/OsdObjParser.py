#!/usr/bin/env python3
# -*- coding:utf-8 -*-

from common import *

class OsdObjParser:
    """
    ## osd 对象代码生成
    - **ini** configparser对象
    - **obj** ini文件中的osd控件名称
    """

    def __init__(self, ini, obj):
        self.posx = -1
        self.posy = -1
        self.width = -1
        self.height = -1
        self.enable = -1
        self.selectable = -1
        self.visible = -1
        self.focused = -1
        self.out_str = ''
        self.ini = ini
        self.obj = obj

    def get_x(self):
        """ get attribute x """
        if 'x' in self.ini.options(self.obj):
            self.posx = self.ini.get(self.obj, 'x')

    def get_y(self):
        """ get attribute y """
        if 'y' in self.ini.options(self.obj):
            self.posy = self.ini.get(self.obj, 'y')

    def get_width(self):
        """ get attribute width """
        if 'width' in self.ini.options(self.obj):
            self.width = self.ini.get(self.obj, 'width')

    def get_height(self):
        """ get attribute height """
        if 'height' in self.ini.options(self.obj):
            self.height = self.ini.get(self.obj, 'height')

    def get_enable(self):
        """ get attribute enable """
        if 'enable' in self.ini.options(self.obj):
            self.enable = self.ini.get(self.obj, 'enable')

    def get_selectable(self):
        """ get attribute selectable """
        if 'selectable' in self.ini.options(self.obj):
            self.selectable = self.ini.get(self.obj, 'selectable')

    def get_visible(self):
        """ get attribute visible """
        if 'visible' in self.ini.options(self.obj):
            self.visible = self.ini.get(self.obj, 'visible')

    def get_focused(self):
        """ get attribute focused """
        if 'focused' in self.ini.options(self.obj):
            self.focused = self.ini.get(self.obj, 'focused')

    def gen_code_x(self):
        """ print code x """
        if self.posx == -1:
            return ''
        else:
            return '    obj_setOffsetX(&self->base, %s);\n' % self.posx

    def gen_code_y(self):
        """ print code y """
        if self.posy == -1:
            return ''
        else:
            return '    obj_setOffsetY(&self->base, %s);\n' % self.posy

    def gen_code_width(self):
        """ print code width """
        if self.width == -1:
            return ''
        else:
            return '    obj_setWidth(&self->base, %s);\n' % self.width

    def gen_code_height(self):
        """ print code height """
        if self.height == -1:
            return ''
        else:
            return '    obj_setHeight(&self->base, %s);\n' % self.height

    def gen_code_enable(self):
        """ print code enable """
        if self.enable == -1:
            return ''
        else:
            return '    obj_setEnable(&self->base, %s);\n' % self.enable

    def gen_code_selectable(self):
        """ print code selectable """
        if self.selectable == -1:
            return ''
        else:
            return '    obj_setSelectable(&self->base, %s);\n' % self.selectable

    def gen_code_visible(self):
        """ print code visible """
        if self.visible == -1:
            return ''
        else:
            return '    obj_setVisible(&self->base, %s);\n' % self.visible

    def gen_code_focused(self):
        """ print code focused """
        if self.focused == -1:
            return ''
        else:
            return '    obj_setFocused(&self->base, %s);\n' % self.focused

    def gen_code_id(self):
        """ print code id """
        return  '    obj_setId(&self->base, %s);\n' \
            % get_obj_id_macro(get_obj_name(self.obj, self.ini.get(self.obj, 'type')))

    def run(self):
        """
        # 创建osd对象初始化代码
        - return 生成代码字符串
        """
        self.get_x()
        self.get_y()
        self.get_width()
        self.get_height()
        self.get_enable()
        self.get_selectable()
        self.get_visible()
        self.get_focused()

        self.out_str = self.out_str + self.gen_code_id()
        self.out_str = self.out_str + self.gen_code_x()
        self.out_str = self.out_str + self.gen_code_y()
        self.out_str = self.out_str + self.gen_code_width()
        self.out_str = self.out_str + self.gen_code_height()
        self.out_str = self.out_str + self.gen_code_enable()
        self.out_str = self.out_str + self.gen_code_selectable()
        self.out_str = self.out_str + self.gen_code_visible()
        self.out_str = self.out_str + self.gen_code_focused()
        # print(self.out_str)

        return self.out_str
