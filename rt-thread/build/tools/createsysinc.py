#!/bin/bash/env python3

import os
import sys

if __name__ == '__main__':
    '''
        create app include sys file.
        argv[1]: rtconfig.h

        output:
          platform/appconfig.h
    '''

    fout = sys.argv[2]
    fin  = sys.argv[1]

    fpin = open(fin, "r")
    fpout = open(fout, 'w')

    fpout.write("#ifndef __APP_CONFIG_H__\n")
    fpout.write("#define __APP_CONFIG_H__\n\n")

    for aLine in fpin:
        if aLine.startswith('#define CONFIG_ARCH_'):
            fpout.write(aLine)
        if aLine.startswith('#define CONFIG_CHIP'):
            fpout.write(aLine)
        if aLine.startswith('#define CONFIG_BOARD'):
            fpout.write(aLine)
        if aLine.startswith('#define FH_FAST_BOOT'):
            fpout.write(aLine)
        if aLine.startswith('#define RT_USING_VFP'):
            fpout.write(aLine)

    fpout.write('\n#endif')
    fpin.close()
    fpout.close()
