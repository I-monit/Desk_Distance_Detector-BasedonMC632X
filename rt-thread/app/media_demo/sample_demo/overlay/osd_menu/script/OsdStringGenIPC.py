#!/usr/bin/env python3
# -*- coding:utf-8 -*-
""" 抽取文本
- 输入: 菜单ini配置文件
- 输出: 菜单的字符串枚举 """
import os
import sys
import platform
import configparser
import codecs

LANG_MESSAGE = './languageMessage.txt'

class OsdStringExtact:
    """
    # 抽取字符串
    - OsdStringExtact.run()
    - 如果dicfile不存在，根据srcfile生成新字典，否则追加字典
    """

    def __init__(self, inifile):
        self.inifile = inifile

    def run(self):
        """
        # 抽取字符串
        """
        if os.path.exists(self.inifile):
            ini = configparser.ConfigParser()
            ini.read(self.inifile)
            (filepath, filename) = os.path.split(self.inifile)
            (filename, extname) = os.path.splitext(filename)
            print('processing %s/%s%s' % (filepath, filename, extname))
        else:
            print('[error] file %s not exist' % self.inifile)
            exit()

        HEAD_STR = "char str_<MENU_NAME>[MAX_COMPONENT_PER_NUM][MAX_STR_LEN] = {\n    "
        text_all = ""
        text_all += HEAD_STR

        for obj in ini.sections():
            name = str(ini.get(obj, 'name'))
            text_all +=  "\"" +name + "\","
            if (ini.get(obj, 'type') == 'combobox' or ini.get(obj, 'type') == 'combobutton'):
                comboname = str(ini.get(obj, 'text'))
                for text in comboname.split(';'):
                    text_all += "\"" +text + "\","
        text_all += "\n};\n"

        # file output
        if not os.path.exists(OUT_PATH):
            os.makedirs(OUT_PATH)


        text_all = text_all.replace('<MENU_NAME>',filename)
        fout = codecs.open(OUT_PATH, 'a', 'utf-8')
        fout.write(text_all)
        fout.close()



# main func
IN_FILE = 'in/awb.ini'
OUT_PATH = 'user/user_string.c'

if __name__ == '__main__':
    if len(sys.argv) == 2:
        IN_FILE = sys.argv[1]

    else:
        print('using default parameter\n')

    OSE = OsdStringExtact(IN_FILE)
    OSE.run()

    exit()
