#!/usr/bin/env python3
# -*- coding:utf-8 -*-
""" 合幷出user_sting.c 代码脚本 """

import os
import sys
import codecs
import time

class OsdStringCombine:
    """
    combine osd string
    """

    def __init__(self, SrcFile, AppendFile):
        self.srcfile = SrcFile
        self.appendfile = AppendFile

    def append_string(self):
        SrcFile = self.srcfile
        AppendFile = self.appendfile
        fsrc = codecs.open(SrcFile, encoding='utf-8', mode='r')
        fappend = codecs.open(AppendFile, encoding='utf-8', mode='r')
        str_combine = ''
        str_combine += fsrc.read()
        str_combine += fappend.read()
        fsrc.close()
        fappend.close()

        # 文件输出
        fout = codecs.open(SrcFile, encoding='gb2312', mode='w')
        fout.write(str_combine)
        fout.close()


if __name__ == "__main__":
    OSC = OsdStringCombine('user/user_string.c', 'user/user_str_index.c')
    OSC.append_string()
    exit()

