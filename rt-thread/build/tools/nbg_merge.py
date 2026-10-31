# -*- coding: utf-8 -*-

import os
import sys
import struct

def combine_files(file_list, output_file):
    with open(output_file, 'wb') as outfile:
        outfile.write(struct.pack('I', len(file_list)))

        file_sizes = []

        for file_name in file_list:
            if os.path.exists(file_name):
                file_size = os.path.getsize(file_name)
                file_sizes.append(file_size)
            else:
                print("文件 {} 不存在，跳过该文件。".format(file_name))
                file_sizes.append(0)

        for size in file_sizes:
            outfile.write(struct.pack('I', size))

        for file_name in file_list:
            if os.path.exists(file_name):
                with open(file_name, 'rb') as infile:
                    outfile.write(infile.read())
            else:
                print("文件 {} 不存在，跳过该文件。".format(file_name))

def main():
    if len(sys.argv) < 3:
        print("用法: python combine_files.py <输出文件> <文件1> <文件2> ... <文件N>")
        sys.exit(1)

    output_file = sys.argv[1]
    file_list = sys.argv[2:]

    combine_files(file_list, output_file)

if __name__ == '__main__':
    main()