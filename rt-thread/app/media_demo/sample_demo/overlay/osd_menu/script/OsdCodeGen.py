#!/usr/bin/env python3
# -*- coding:utf-8 -*-
""" 自动生成 OSD菜单 代码脚本 """
import time

import OsdMenuParser
import OsdButtonParser
import OsdComboboxParser
import OsdScrollbarParser
import OsdTextboxParser
import OsdCombobuttonParser

from common import *


class OsdCodeGen:
    """
    - osd code generator
    """

    def __init__(self, ini):
        self.ini = ini
        self.sections = self.ini.sections()
        self.filename = 'menu_' + self.get_menu_name()

    def gen_src_file(self):
        """
        # 生成 源 文件的定义和初始化部分
        """
        code = self.gen_c_head_code()
        code += self.gen_declaration()
        code += self.gen_obj_init_code()
        code += self.gen_menu_init_code()
        return code

    def gen_callback_src_file(self):
        """
        # 生成 callback 文件
        """
        code = self.gen_c_callback_head_code()
        code += self.gen_callback_code()
        return code

    def gen_callback_header_file(self):
        """
        # 生成 callback的头 文件
        """
        code = self.gen_callback_head_file_code()
        return code

    def gen_header_file(self):
        """
        # 生成 头 文件
        """
        code = self.gen_head_file_code()
        return code

    def gen_declaration(self):
        "- 生成控件静态声明"
        str_obj_declear = ''
        str_num_define = '\n#define NUM_BASE (0)\n'
        str_id_define = '\n#define ID_BASE (0)\n'
        obj_name_prev = 'BASE'
        obj_num_prev = 'NUM_BASE'

        for obj in self.sections:
            obj_type = self.ini.get(obj, 'type')
            obj_name = get_obj_name(obj, obj_type)
            obj_num = get_obj_num_macro(obj_name)
            obj_str_num = self.get_obj_string_number(obj)

            str_obj_declear += self.get_obj_declaration(
                obj_name, obj_type) + '\n'

            str_num_define += self.get_obj_number_define(
                obj_name, obj_str_num) + '\n'

            str_id_define += self.get_obj_id_define(
                obj_name, obj_name_prev, obj_num_prev) + '\n'
            obj_name_prev = obj_name
            obj_num_prev = obj_num

        str_comment = "\
/**************************************\n\
 * 控件声明\n\
 *************************************/\n\
"
        str_declaration = str_comment + str_obj_declear + \
            str_num_define + str_id_define + '\n'
        # print(str_obj_declear)
        # print(str_num_define)
        # print(str_id_define)
        return str_declaration

    def get_menu_name(self):
        "- 获取菜单名"
        return self.sections[0]

    def get_filename(self):
        "- 获取菜单文件名"
        return self.filename

    @staticmethod
    def get_obj_declaration(obj_name, obj_type):
        "- 获取控件静态声明"
        str_template = '<TYPE>_t <OBJ_NAME>;'
        return str_template.replace('<TYPE>', obj_type).replace('<OBJ_NAME>', obj_name)

    def get_obj_string_number(self, obj):
        "- 获取控件字符串数量"
        obj_type = self.ini.get(obj, 'type')
        number = 1
        if (obj_type == 'combobox' or obj_type == 'combobutton'):
            number = number + \
                int(OsdComboboxParser.get_combobox_max_num(self.ini, obj))
        return str(number)

    @staticmethod
    def get_obj_number_define(obj_name, number):
        "- 获取控件字符串数量宏定义"
        str_template = '#define NUM_<OBJ> (<NUM>)'
        return str_template.replace('<OBJ>', obj_name).replace('<NUM>', number)

    @staticmethod
    def get_obj_id_define(obj_name, obj_name_prev, obj_num_prev):
        "- 获取控件ID定义"
        str_template = '#define ID_<OBJ> (ID_<OBJ_PREV> + <NUM_PREV>)'
        return str_template.replace('<OBJ>', obj_name).replace(
            '<OBJ_PREV>', obj_name_prev).replace('<NUM_PREV>', obj_num_prev)

    def gen_obj_init_code(self):
        "## 生成控件初始化代码"
        str_obj_init = ''
        str_comment = "\
/**************************************\n\
 * 控件初始化\n\
 *************************************/\n\
"
        type2parser = {
            'menu': OsdMenuParser.OsdMenuParser,
            'button': OsdButtonParser.OsdButtonParser,
            'combobox': OsdComboboxParser.OsdComboboxParser,
            'scrollbar': OsdScrollbarParser.OsdScrollbarParser,
            'textbox': OsdTextboxParser.OsdTextboxParser,
            'combobutton': OsdCombobuttonParser.OsdCombobuttonParser,
        }
        for obj in self.sections:
            obj_type = self.ini.get(obj, 'type')
            parser = type2parser[obj_type](self.ini, obj)
            str_obj_init += parser.run()

        return str_comment + str_obj_init

    def gen_menu_init_code(self):
        "## 生成菜单初始化代码"
        str_menu_init = ''
        str_comment = "\
/**************************************\n\
 * 菜单初始化\n\
 *************************************/\n\
"
        # object init
        for obj in self.sections:
            obj_type = self.ini.get(obj, 'type')
            obj_name = get_obj_name(obj, obj_type)
            code = '    <OBJNAME>_init();\n'.replace('<OBJNAME>', obj_name)
            str_menu_init += code
        str_menu_init += '\n'

        # object link
        prev_obj = 'null'
        for obj in self.sections:
            obj_type = self.ini.get(obj, 'type')
            obj_name = get_obj_name(obj, obj_type)
            if prev_obj == 'null':
                prev_obj = obj_name
                continue

            if obj == self.sections[1]:
                code = '    obj_adopt(&<OBJPREV>.base, &<OBJNAME>.base);\n'\
                    .replace('<OBJPREV>', prev_obj).replace('<OBJNAME>', obj_name)
            else:
                code = '    obj_append(&<OBJPREV>.base, &<OBJNAME>.base);\n'\
                    .replace('<OBJPREV>', prev_obj).replace('<OBJNAME>', obj_name)
            prev_obj = obj_name
            str_menu_init += code
        str_menu_init += '\n'

        # object callback
        code = '    <FILENAME>_register();\n'.replace('<FILENAME>', self.filename)
        str_menu_init += code

        menu_name = get_obj_name(self.get_menu_name(), 'menu')
        str_head = 'void %s_initialize(void) {\n' % menu_name
        str_tail = '}\n'

        return str_comment + str_head + str_menu_init + str_tail

    def gen_c_head_code(self):
        "## 生成头部代码"
        str_comment = "\
/**\n\
 * File      : <FILENAME>.c\n\
 * This file is automatically generated by menu_create.py\n\
 * UNCOMMENT AND MODIFY CALLBACK FUNCTION AS REQUIRED\n\
 *\n\
 * Copyright (c) 2017 Shanghai Fullhan Microelectronics Co., Ltd.\n\
 * All rights reserved\n\
 *\n\
 * Script Version: <VERSION>\n\
 * Generated at: <DATE>\n\
 */\n\
#include \"<FILENAME>.h\"\n\
#include \"../callback/<FILENAME>_callback.h\"\n\
#include \"../menu/configure.h\"\n\
#include \"../user_menu.h\"\n\
\n"
        out_str = str_comment.replace('<FILENAME>', self.filename)\
            .replace('<DATE>', time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))\
            .replace('<VERSION>', '3.0')

        return out_str

    def gen_c_callback_head_code(self):
        "## 生成callback头部代码"
        str_comment = "\
/**\n\
 * File      : <FILENAME>_callback.c\n\
 * This file is automatically generated by menu_create.py\n\
 * UNCOMMENT AND MODIFY CALLBACK FUNCTION AS REQUIRED\n\
 *\n\
 * Copyright (c) 2017 Shanghai Fullhan Microelectronics Co., Ltd.\n\
 * All rights reserved\n\
 *\n\
 * Script Version: <VERSION>\n\
 * Generated at: <DATE>\n\
 */\n\
#include \"../menu/<FILENAME>.h\"\n\
#include \"<FILENAME>_callback.h\"\n\
#include \"../menu/configure.h\"\n\
\n"
        out_str = str_comment.replace('<FILENAME>', self.filename)\
            .replace('<DATE>', time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))\
            .replace('<VERSION>', '3.0')
        return out_str

    def gen_head_file_code(self):
        "## 生成 h 文件代码"
        pattern = "\
/**\n\
 * File      : <FILENAME>.h\n\
 * This file is automatically generated by menu_create.py\n\
 * PLEASE DO NOT MODIFY THIS FILE DIRECTLY\n\
 *\n\
 * Copyright (c) 2017 Shanghai Fullhan Microelectronics Co., Ltd.\n\
 * All rights reserved\n\
 *\n\
 * Script Version: <VERSION>\n\
 * Generated at: <DATE>\n\
 */\n\
#ifndef _<FILENAME_UP>_H_\n\
#define _<FILENAME_UP>_H_\n\
\n\
#include \"../../component/osd_components.h\"\n\
\n"
        head_str = pattern.replace('<FILENAME>', self.filename)\
            .replace('<DATE>', time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))\
            .replace('<VERSION>', '3.0')\
            .replace('<FILENAME_UP>', self.filename.upper())

        extern_str = ''
        for obj in self.sections:
            obj_type = self.ini.get(obj, 'type')
            obj_name = get_obj_name(obj, obj_type)
            extern_str += 'extern ' + self.get_obj_declaration(obj_name, obj_type) + '\n'
        extern_str += '\n'

        menu_name = get_obj_name(self.get_menu_name(), 'menu')
        func_str = 'void %s_initialize(void);\n\n' % menu_name

        tail_str = '#endif  // _<FILENAME_UP>_H_\n'\
            .replace('<FILENAME_UP>', self.filename.upper())

        return head_str + extern_str + func_str + tail_str

    def gen_callback_head_file_code(self):
        "## 生成 callback的h 文件代码"
        pattern = "\
/**\n\
 * File      : <FILENAME>_callback.h\n\
 * This file is automatically generated by menu_create.py\n\
 * PLEASE DO NOT MODIFY THIS FILE DIRECTLY\n\
 *\n\
 * Copyright (c) 2017 Shanghai Fullhan Microelectronics Co., Ltd.\n\
 * All rights reserved\n\
 *\n\
 * Script Version: <VERSION>\n\
 * Generated at: <DATE>\n\
 */\n\
#ifndef _<FILENAME_UP>_CALLBACK_H_\n\
#define _<FILENAME_UP>_CALLBACK_H_\n\
\n\
\n"
        head_str = pattern.replace('<FILENAME>', self.filename)\
            .replace('<DATE>', time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))\
            .replace('<VERSION>', '3.0')\
            .replace('<FILENAME_UP>', self.filename.upper())

        callback_str = ''
        code = 'void <FILENAME>_register();\n'.replace('<FILENAME>', self.filename)
        callback_str += code

        type2callback = {
            'menu': '\
// void <OBJNAME>_onDraw(void *self);\n',
            'button': '\
// void <OBJNAME>_onClick(void *self);\n',
            'combobox': '\
// void <OBJNAME>_onDataChange(void *self);\n\
// void <OBJNAME>_onDraw(void *self);\n',
            'scrollbar': '\
// void <OBJNAME>_onDataChange(void *self);\n\
// void <OBJNAME>_onDraw(void *self);\n',
            'textbox': '',
            'combobutton': '\
// void <OBJNAME>_onDataChange(void *self);\n\
// void <OBJNAME>_onDraw(void *self);\n',
        }
        for obj in self.sections:
            obj_type = self.ini.get(obj, 'type')
            obj_name = get_obj_name(obj, obj_type)
            code = type2callback[obj_type].replace('<OBJNAME>', obj_name)
            callback_str += code

        tail_str = '\n#endif  // _<FILENAME_UP>_CALLBACK_H_\n'\
            .replace('<FILENAME_UP>', self.filename.upper())

        return head_str + callback_str + tail_str

    def gen_callback_code(self):
        "## 生成回调代码"
        str_callback = ''

        str_register = 'void <FILENAME>_register() {\n'.replace('<FILENAME>', self.filename)
        type2callback = {
            'menu': '    // menu_registerDrawCallback(&<OBJNAME>, <OBJNAME>_onDraw);\n',
            'button': '    // button_registerClickCallback(&<OBJNAME>, <OBJNAME>_onClick);\n',
            'combobox': '    // combobox_registerDataChangeCallback(&<OBJNAME>, <OBJNAME>_onDataChange);\n\
    // combobox_registerGetValueCallback(&<OBJNAME>, <OBJNAME>_onDraw);\n',
            'scrollbar': '    // scrollbar_registerDataChangeCallback(&<OBJNAME>, <OBJNAME>_onDataChange);\n\
    // scrollbar_registerGetValueCallback(&<OBJNAME>, <OBJNAME>_onDraw);\n',
            'textbox': '',
            'combobutton': '    // combobutton_registerDataChangeCallback(&<OBJNAME>, <OBJNAME>_onDataChange);\n\
    // combobutton_registerGetValueCallback(&<OBJNAME>, <OBJNAME>_onDraw);\n',
        }

        for obj in self.sections:
            obj_type = self.ini.get(obj, 'type')
            obj_name = get_obj_name(obj, obj_type)
            code = type2callback[obj_type].replace('<OBJNAME>', obj_name)
            str_register += code
        str_register += '}\n\n'


        str_comment = "\
/**************************************\n\
 * 事件响应\n\
 *************************************/\n\
"
        # object callback
        type2callback = {
            'menu': '\
// void <OBJNAME>_onDraw(void *self) {\n\
//     return;\n\
// }\n\n',
            'button': '\
// void <OBJNAME>_onClick(void *self) {\n\
//     return;\n\
// }\n\n',
            'combobox': '\
// void <OBJNAME>_onDataChange(void *self) {\n\
//     return;\n\
// }\n\n\
// void <OBJNAME>_onDraw(void *self) {\n\
//     return;\n\
// }\n\n',
            'scrollbar': '\
// void <OBJNAME>_onDataChange(void *self) {\n\
//     return;\n\
// }\n\n\
// void <OBJNAME>_onDraw(void *self) {\n\
//     return;\n\
// }\n\n',
            'textbox': '',
            'combobutton': '\
// void <OBJNAME>_onDataChange(void *self) {\n\
//     return;\n\
// }\n\n\
// void <OBJNAME>_onDraw(void *self) {\n\
//     return;\n\
// }\n\n',
        }

        for obj in self.sections:
            obj_type = self.ini.get(obj, 'type')
            obj_name = get_obj_name(obj, obj_type)
            code = type2callback[obj_type].replace('<OBJNAME>', obj_name)
            str_callback += code

        return str_register + str_comment + str_callback

