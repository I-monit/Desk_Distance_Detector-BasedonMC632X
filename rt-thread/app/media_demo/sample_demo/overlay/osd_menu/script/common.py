#!/usr/bin/env python3
# -*- coding:utf-8 -*-

def camel_case(string):
    """- 首字大写 """
    return string[0].upper() + string[1:]


def get_obj_name(name, obj_type):
    "- <obj_type><Name>"
    str_template = '<TYPE><OBJ>'
    return str_template.replace('<TYPE>', obj_type).replace('<OBJ>', camel_case(name))


def get_obj_num_macro(obj_name):
    "- NUM_<OBJNAME>"
    str_template = 'NUM_<OBJ>'
    return str_template.replace('<OBJ>', obj_name)


def get_obj_id_macro(obj_name):
    "- ID_<OBJNAME>"
    str_template = 'ID_<OBJ>'
    return str_template.replace('<OBJ>', obj_name)
