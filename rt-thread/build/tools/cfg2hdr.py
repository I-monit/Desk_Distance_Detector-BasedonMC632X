import os
import sys
import shutil

def mk_rtconfig(filename, dstpath):
    try:
        config = open(filename, 'r')
    except:
        print('open config:%s failed' % filename)
        return

    hdrfile = ".config.h"
    rtconfig = open(hdrfile, 'w')
    rtconfig.write('#ifndef RT_CONFIG_H__\n')
    rtconfig.write('#define RT_CONFIG_H__\n\n')

    empty_line = 1
    soc_name = ''
    brd_name = ''

    for line in config:
        line = line.lstrip(' ').replace('\n', '').replace('\r', '')

        if len(line) == 0: continue

        if line[0] == '#':
            if len(line) == 1:
                if empty_line:
                    continue

                rtconfig.write('\n')
                empty_line = 1
                continue

            comment_line = line[1:]
            if line.startswith('# CONFIG_'): line = ' ' + line[9:]
            else: line = line[1:]

            rtconfig.write('/*%s */\n' % line)
            empty_line = 0
        else:
            empty_line = 0
            setting = line.split('=')
            #
            # for external settings, define it in rtconfig.h?
            #
            # if setting.find('components packages') > 0:
            #     break
            if len(setting) >= 2:
                if setting[0].startswith('CONFIG_'):
                    setting[0] = setting[0][7:]

                if type(setting[0]) == type('a') and (setting[0].endswith('_PATH') or setting[0].endswith('_VER')):
                    continue

                if setting[1] == 'y':
                    rtconfig.write('#define %s\n' % setting[0])
                    if setting[0].startswith('CONFIG_CHIP'):
                        soc_name = setting[0].split('_')[2].lower()
                    if setting[0].startswith('CONFIG_BOARD'):
                        brd_name = setting[0].split('_')[2].lower() + '_board'
                else:
                    rtconfig.write('#define %s %s\n' % (setting[0], setting[1]))

    rtconfig.write('\n')
    rtconfig.write('#endif\n')
    rtconfig.close()

    if (len(soc_name) == 0 or len(brd_name) == 0) and len(dstpath) == 0:
        print(dstpath)
        print("bad config file, NO SOC/BOARD defined!");

    if len(dstpath) == 0:
        dstpath = 'platform/' + soc_name + '/' + brd_name
        if not os.path.exists(dstpath):
            print('bad config file, bad soc or board!')
            return
        shutil.move(hdrfile, dstpath + '/rtconfig.h')
    else:
        shutil.move(hdrfile, dstpath)

if __name__ == '__main__':
    if len(sys.argv) > 2:
        mk_rtconfig(".config", sys.argv[2])
    else:
        mk_rtconfig(".config", "")
