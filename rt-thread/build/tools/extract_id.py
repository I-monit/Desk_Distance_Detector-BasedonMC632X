import re
import sys
import os

def extract_chip_id(config_file):
    with open(config_file, 'r') as file:
        for line in file:
            match = re.match(r'CONFIG_CONFIG_CHIP_(\w+)=y', line)
            if match:
                return match.group(1).upper()
    return None

def extract_arch(config_file):
    with open(config_file, 'r') as file:
        for line in file:
            match = re.match(r'CONFIG_CONFIG_ARCH_(\w+)=y', line)
            if match and 'FULLHAN' not in match.group(1):
                return match.group(1).upper()
    return None

def extract_arch_merge_makefile(config_file, makefile):
    id = "NULL"
    with open(config_file, 'r') as file:
        for line in file:
            match = re.match(r'CONFIG_CONFIG_ARCH_(\w+)=y', line)
            if match and 'FULLHAN' not in match.group(1):
                id = match.group(1).upper()

    with open(makefile, 'r') as file:
        lines = file.readlines()
        for i, line in enumerate(lines):
            if 'OS ?= 'in line:
                lines[i] = 'OS ?= RTT\n'
                with open(makefile, 'w') as file:
                    file.writelines(lines)
    return id

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python extract_chip_id.py <config_file>")
        sys.exit(1)

    type = sys.argv[1]
    config_file = sys.argv[2]

    if not os.path.isfile(config_file):
        sys.exit(1)

    id = "NULL"
    if type == "chip":
        id = extract_chip_id(config_file)

    if type == "arch":
        id = extract_arch(config_file)

    if len(sys.argv) == 4:
        makefile = sys.argv[3]
        if type == "merge":
            if not os.path.isfile(makefile):
                sys.exit(0)
            id = extract_arch_merge_makefile(config_file, makefile)

    print(id)
