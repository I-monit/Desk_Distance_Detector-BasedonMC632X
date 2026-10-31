#!/usr/bin/env python3
import os
import sys
import struct
import re
import string

def extract_sensor_names(config_file):
    sensor_names = []
    pattern = re.compile(r'#define SENSOR_NAME_G\d+ "([^"]+)"')

    with open(config_file, 'r') as file:
        for line in file:
            match = pattern.search(line)
            if match:
                sensor_names.append(match.group(1))

    return sensor_names

def gener_hex_file_hdr(hex_name, path): #generate hex_file_hdr

    hex_size   = []
    sensor_num = len(hex_name)

    for i in range(0, sensor_num):
        hex_size.append(os.path.getsize(path + hex_name[i]))

    total_len  = 16 + sensor_num * 48 + sum(hex_size)
    magic      = 0x6ad7bfc5
    reserved   = 0

    byte_in =  struct.pack('<I', total_len)
    byte_in += struct.pack('<I', magic)
    byte_in += struct.pack('<I', reserved)
    byte_in += struct.pack('<I', reserved)

    return byte_in

def gener_hex_file_entry(hex_name, byte_in, path): #generate hex_file_entry

    hex_size   = [0]
    cur_size   = 0
    sensor_num = len(hex_name)

    for i in range(0, sensor_num):
        hex_size.append(os.path.getsize(path + hex_name[i]))

    for num in range(0, sensor_num):
        cur_size  += hex_size[num]
        offset    = 16 + sensor_num * 48 + cur_size
        length    = hex_size[num + 1]
        magic     = 0x6ad7bfc5
        file_name = bytes(hex_name[num], encoding = 'utf-8')

        byte_in += struct.pack('<I', offset)
        byte_in += struct.pack('<I', length)
        byte_in += struct.pack('<I', magic)
        byte_in += struct.pack('<36s', file_name)

    return byte_in

def gener_hex_file(hex_name, byte_in, path, path_write): #generate .hex
    hex_file   = open(path_write, 'wb')
    sensor_num = len(hex_name)

    for i in range(0, sensor_num):
        sensor_file =  open(path + hex_name[i], 'rb')
        byte_in     += sensor_file.read()
        sensor_file.close()

    hex_file.write(byte_in)
    hex_file.close()

def hex2headfile(inFile, outFile, destFolder):

    fileIn = open(inFile, "rb")
    fileOut = open(destFolder + "/" + outFile, "w")
    fileSize = os.path.getsize(inFile)

    if "wdr" in inFile:
        wdr = "_wdr"
    else:
        wdr = ""

    if "night" in inFile:
        day = "_night"
    else:
        day = ""

    string = "#ifndef __SENSOR_HEX_RTT_H__\n"
    string += "#define __SENSOR_HEX_RTT_H__\n"
    string += "char g_sensor_hex_array" + wdr + day + "[] = {\n    "

    for i in range(0,fileSize):
        num = fileIn.read(1)
        num = hex(ord(num))
        num = "0x" + num[2:].rjust(2, '0')
        string += str(num)
        if ((i + 1) % 4) == 0 and i != 0:
            if i + 1 < fileSize:
                string += ","
                string += "  // 0x" + hex(i - 3)[2:].rjust(4,'0')
                string += "\n    "
            else:
                string += ","
                string += "  // 0x" + hex(i - 3)[2:].rjust(4,'0')
        else:
            string += ", "
    string += "\n};\n"

    string += "#endif\n"

    fileOut.write(string)
    fileIn.close()
    fileOut.close()

if __name__ == '__main__':

    rtconfig_file = sys.argv[1]
    chip = sys.argv[2].lower()
    no_find = []
    find_flag = 0
    name_list = []

    hex_name = extract_sensor_names(rtconfig_file) #get sensor interface type

    path2hex = os.path.abspath(sys.argv[3] + '/lib/' + chip + '/lib/sensor_hex') + '/' #path to hex
    name_all = os.listdir(path2hex)

    for i in range(0, len(hex_name)): #get sensor name_list
        for j in range(0, len(name_all)):
            if hex_name[i] == name_all[j][0:len(hex_name[i])]:
                name_list.append(name_all[j])
                find_flag = 1

        if find_flag == 0:
            no_find.append(hex_name[i])
        find_flag = 0

    for i in range(0, len(no_find)):
        print('\n Warning: can not find %s!!!' % no_find[i])

    byte_in        = gener_hex_file_hdr(name_list, path2hex)
    byte_in        = gener_hex_file_entry(name_list, byte_in, path2hex)
    path_in        = os.path.abspath(os.getcwd() + '/sample_common/all_sensor_hex.bin')
    path_out       = os.path.abspath(os.getcwd() + '/sample_common/')
    gener_hex_file(name_list, byte_in, path2hex, path_in)
    hex2headfile(path_in, 'all_sensor_array.h', path_out)
