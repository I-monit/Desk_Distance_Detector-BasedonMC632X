#!/usr/bin/env python3
# -*- coding:utf-8 -*-
""" 自动生成 OSD菜单 代码脚本 """
import os
import sys
import platform
import configparser
import codecs

import OsdCodeGen

IN_FILE = 'in/main.ini'
OUT_PATH_MENU = './menu'
OUT_PATH_CALLBACK_MENU = './callback'
if __name__ == '__main__':

    # print(platform.python_version())

    if len(sys.argv) == 4:
        IN_FILE = sys.argv[1]
        OUT_PATH_MENU = sys.argv[2]
        OUT_PATH_CALLBACK_MENU = sys.argv[3]

    print('processing %s' % IN_FILE)

    INI = configparser.ConfigParser()
    INI.read(IN_FILE)

    OCD = OsdCodeGen.OsdCodeGen(INI)

    if not os.path.exists(OUT_PATH_MENU):
        os.makedirs(OUT_PATH_MENU)
    if not os.path.exists(OUT_PATH_CALLBACK_MENU):
        os.makedirs(OUT_PATH_CALLBACK_MENU)

    OUT_FILE = OUT_PATH_MENU + '/'+ OCD.get_filename() + '.c'
    FOUT = codecs.open(OUT_FILE, 'w+', 'utf-8')
    FOUT.write(OCD.gen_src_file())
    FOUT.close()

    OUT_FILE = OUT_PATH_MENU + '/'+ OCD.get_filename() + '.h'
    FOUT = codecs.open(OUT_FILE, 'w+', 'utf-8')
    FOUT.write(OCD.gen_header_file())
    FOUT.close()

    OUT_FILE = OUT_PATH_CALLBACK_MENU + '/'+ OCD.get_filename() + '_callback.c'
    FOUT = codecs.open(OUT_FILE, 'w+', 'utf-8')
    FOUT.write(OCD.gen_callback_src_file())
    FOUT.close()

    OUT_FILE = OUT_PATH_CALLBACK_MENU + '/'+ OCD.get_filename() + '_callback.h'
    FOUT = codecs.open(OUT_FILE, 'w+', 'utf-8')
    FOUT.write(OCD.gen_callback_header_file())
    FOUT.close()
