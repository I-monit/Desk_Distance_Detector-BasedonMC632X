#!/usr/bin/env python3
# -*- encoding: utf-8 -*-

import os
import sys
import struct
import re
import string

def find_sensor_hex(rtconfig_file):
    '''
    找到rtconfig.h中sensor对应的hex，并生成hex_name数组

    rtconfig_file: rtconfig.h

    return: defined sensor name list/chip name

    '''
    _config = open(rtconfig_file, 'r')
    sensor_config = 0
    sensor_list = []
    chip = ''

    for line in _config:

        if 'CONFIG_CHIP' in line:
            if line[0] == '#':
                chip = line.split('_')[-1]
                chip = chip.replace('\n', '').replace('\r', '').lower()

        if 'FH_ENABLE_VIDEO' in line:
            sensor_config = 1

        if 'EMBED_SENSOR_HEX_PARAM' in line:
            sensor_config = 0

        if sensor_config and line.startswith('#define RT_'):
            setting = line.split('_')
            setting[2] = setting[2].replace('\n', '').replace('\r', '')
            if len(setting) > 3:
                setting[3] = setting[3].replace('\n', '').replace('\r', '')
                sensor_list.append(setting[2].lower() + '_' + setting[3].lower())
            else:
                sensor_list.append(setting[2].lower())

    _config.close()

    return sensor_list, chip

def gener_hex_file_hdr(hex_name):
    '''
    generate hex_file_hdr

    hex_name: sensor parameter filename

    return: header bytes
    '''

    hex_size   = []
    sensor_num = len(hex_name)

    for i in range(0, sensor_num):
        hex_size.append(os.path.getsize(hex_name[i]))

    total_len  = 16 + sensor_num * 48 + sum(hex_size)
    magic      = 0x6ad7bfc5

    byte_in =  struct.pack('<IIII', total_len, magic, sensor_num, 0)

    return byte_in

# format: 
#   1. char[16]: sensor_name
#   2. char[16]: property
#   3. offset
#   4. length
#   5. magic
#   6. reserved
def gener_hex_file_entry(hex_name, byte_in, sensor_name):
    '''
    generate hex_file_entry

    hex_name: sensor parameter file name
    byte_in : input buffer

    return  : byte buffer
    '''

    hex_size   = [0]
    cur_size   = 0
    sensor_num = len(hex_name)

    for hexf in hex_name:
        hex_size.append(os.path.getsize(hexf))

    for num in range(0, sensor_num):
        cur_size  += hex_size[num]
        offset    = 16 + sensor_num * 48 + cur_size
        length    = hex_size[num + 1]
        magic     = 0x6ad7bfc5
        fname = os.path.basename(hex_name[num])
        sname = ""
        pname = ""
        for sens in sensor_name:
            if fname.startswith(sens):
                sname = sens
                aname = fname[len(sens) + 1:]
                alist = aname.split('_')
                if len(alist) == 1:
                    pname = "genr_"
                else:
                    for i in range(len(alist) - 1):
                        pname += alist[i] + "_"
        pname = pname[0:len(pname) - 1]

        sens_name = bytes(sname, encoding = 'utf-8')
        prop_name = bytes(pname, encoding = 'utf-8')

        byte_in += struct.pack('<16s', sens_name)
        byte_in += struct.pack('<16s', prop_name)
        byte_in += struct.pack('<IIII', offset, length, magic, 0)

    return byte_in

def gener_hex_file(hex_name, byte_in, path):
    '''
    generate .hex

    hex_name: sensor parameter file name
    byte_in : input buffer
    path    : sensor parameter file path

    return  : None
    '''
    path_write = os.path.abspath(path + '/sensor_hex.bin')
    hex_file   = open(path_write, 'wb')
    sensor_num = len(hex_name)

    for aFile in hex_name:
        sensor_file =  open(aFile, 'rb')
        byte_in     += sensor_file.read()
        sensor_file.close()

    hex_file.write(byte_in)
    hex_file.close()

if __name__ == '__main__':
    '''
    merge sensor param file into one binary file, according to config file.

    argv[1]: the rtconfig.h
    argv[2]: output directory
    '''
    rtconfig_file = sys.argv[1]
    no_find = []
    name_list = []

    hex_name, chip = find_sensor_hex(rtconfig_file) # get sensor interface type
    path2hex = os.path.abspath(os.getcwd() + '/platform/sensor/sensor_param/' + chip) + '/' # path to hex

    for sensor in hex_name: # get sensor name_list
        if os.path.isdir(path2hex + sensor):
            name_all = os.listdir(path2hex + sensor)
            for aName in name_all:
                name_list.append(path2hex + sensor + '/' + aName)
        
        else:
            no_find.append(sensor)

    for i in range(0, len(no_find)):
        print('\n Warning: can not find %s!!!' % no_find[i])

    byte_in        = gener_hex_file_hdr(name_list)
    byte_in        = gener_hex_file_entry(name_list, byte_in, hex_name)
    gener_hex_file(name_list, byte_in, sys.argv[2])
