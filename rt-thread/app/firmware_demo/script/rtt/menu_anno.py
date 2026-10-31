#!/usr/bin/env python3

import os
import sys
import re

format_wh_mapping = {
    "720P15": (1280, 720),
    "720P20": (1280, 720),
    "720P25": (1280, 720),
    "720P30": (1280, 720),
    "720P90": (1280, 720),
    "960P25": (1280, 960),
    "960P30": (1280, 960),
    "1080P15": (1920, 1080),
    "1080P20": (1920, 1080),
    "1080P25": (1920, 1080),
    "1080P30": (1920, 1080),
    "1080P60": (1920, 1080),
    "1200P30": (1600, 1200),
    "1536X1536P15": (1536, 1536),
    "1536X1536P25": (1536, 1536),
    "1536X1536P30": (1536, 1536),
    "1536P15": (2048, 1536),
    "1536P25": (2048, 1536),
    "1536P30": (2048, 1536),
    "1536P40": (2048, 1536),
    "400WP10": (2560, 1440),
    "400W12P5": (2560, 1440),
    "400WP15": (2560, 1440),
    "400WP20": (2560, 1440),
    "400WP25": (2560, 1440),
    "400WP30": (2560, 1440),
    "500W12P5": (2560, 1944),
    "500WP15": (2560, 1944),
    "500WP20": (2560, 1944),
    "500WP25": (2560, 1944),
    "800W12P5": (3840, 2448),
    "800WP15": (3840, 2160),
    "800WP20": (3840, 2160),
    "800WP25": (3840, 2160),
    "800WP30": (3840, 2160),
    "270P240": (480, 270),
    "540P120": (960, 540),
    "540P180": (960, 540),
    "2304X1296P10": (2304, 1296),
    "2304X1296P15": (2304, 1296),
    "2304X1296P25": (2304, 1296),
    "2304X1296P30": (2304, 1296),
    "192X192P220": (192, 192),
    "384X384P110": (384, 384),
    "640X480P30": (640, 480),
    "3072X1728P15": (3072, 1728),
    "3072X1728P25": (3072, 1728),
    "2592X1440P15": (2592, 1440),
    "3840X2160P15": (3840, 2160),
    "2688X1520P15": (2688, 1520),
    "2688X1520P20": (2688, 1520),
    "2688X1520P25": (2688, 1520),
    "2688X1520P30": (2688, 1520),
    "2592X1952P30": (2592, 1952),
    "352X288P30": (352, 288),
    "2240X2016P30": (2240, 2016),
    "600W12P5": (3200, 1800),
    "600WP15": (3200, 1800),
    "600WP20": (3200, 1800),
    "600WP25": (3200, 1800),
    "600WP30": (3200, 1800),
    "2464X1760P30": (2464, 1760),
    "3840X1080P30": (3840, 1080),
    "4096X128P25": (4096, 128),
    "4096X128P30": (4096, 128),
    "3264X2448P15": (3264, 2448),
    "5120X2160P15": (5120, 2160),
    "1920X1632P20": (1920, 1632),
    "1920X1632P30": (1920, 1632),
    "2880X1620P15": (2880, 1620),
    "2880X1620P20": (2880, 1620),
    "2880X1620P25": (2880, 1620),
    "2880X1620P30": (2880, 1620),
}

def get_format_wh(format):
    return format_wh_mapping.get(format, (0, 0))

def write_sensor(string):
    pattern = r'^FH_USING_([A-Za-z0-9]+)_([A-Za-z0-9]+)_G([0-9])$'
    match = re.match(pattern, string)
    if match:
        sensor, mipi, group = match.groups()
        sensor_name = "SENSOR_NAME_G{} {}".format(group, '"{}"'.format("{}_{}".format(sensor.lower(), mipi.lower())))
        return sensor_name
    return None

def write_sensor_format(string):
    wdr_s = r'^FH_APP_USING_FORMAT_([A-Za-z0-9]+)_WDR_G([0-9])$'
    line_s = r'^FH_APP_USING_FORMAT_([A-Za-z0-9]+)_G([0-9])$'
    slave_s = r'^FH_APP_USING_FORMAT_([A-Za-z0-9]+)_SLAVE_G([0-9])$'
    selfsync_s = r'^FH_APP_USING_FORMAT_([A-Za-z0-9]+)_SELFSYNC_G([0-9])$'

    match = re.match(line_s, string)
    if match:
        format, group = match.groups()
        w, h = get_format_wh(format)
        isp_format = "ISP_FORMAT_G{} FORMAT_{}".format(group, format)
        isp_vo_w = "VI_INPUT_WIDTH_G{} {}".format(group, w)
        isp_vo_h = "VI_INPUT_HEIGHT_G{} {}".format(group, h)
        return isp_format, isp_vo_w, isp_vo_h

    match = re.match(wdr_s, string)
    if match:
        format, group = match.groups()
        w, h = get_format_wh(format)
        isp_format = "ISP_FORMAT_G{} FORMAT_WDR_{}".format(group, format)
        isp_vo_w = "VI_INPUT_WIDTH_G{} {}".format(group, w)
        isp_vo_h = "VI_INPUT_HEIGHT_G{} {}".format(group, h)
        return isp_format, isp_vo_w, isp_vo_h

    match = re.match(slave_s, string)
    if match:
        format, group = match.groups()
        w, h = get_format_wh(format)
        isp_format = "ISP_FORMAT_G{} FORMAT_{}_SLAVE".format(group, format)
        isp_vo_w = "VI_INPUT_WIDTH_G{} {}".format(group, w)
        isp_vo_h = "VI_INPUT_HEIGHT_G{} {}".format(group, h)
        return isp_format, isp_vo_w, isp_vo_h

    match = re.match(selfsync_s, string)
    if match:
        format, group = match.groups()
        w, h = get_format_wh(format)
        isp_format = "ISP_FORMAT_G{} FORMAT_{}_SELFSYNC".format(group, format)
        isp_vo_w = "VI_INPUT_WIDTH_G{} {}".format(group, w)
        isp_vo_h = "VI_INPUT_HEIGHT_G{} {}".format(group, h)
        return isp_format, isp_vo_w, isp_vo_h

    return None

def write_config(config_filename, filename, chipname):
    config_h = open(filename, 'w')
    config_h.write('#ifndef __CONFIG_H__\n')
    config_h.write('#define __CONFIG_H__\n\n')

    config_h.write('#define JPEG_CHN_OFFSET 3\n')
    if chipname == 'FH8866':
        config_h.write('#define MAX_VPU_CHN_NUM 4\n')
        config_h.write('#define MAX_GRP_NUM 3\n')
    if chipname == 'FH8852V301_FH8662V100' or chipname == 'MC632X':
        config_h.write('#define MAX_VPU_CHN_NUM 3\n')
        config_h.write('#define MAX_GRP_NUM 2\n')
    if chipname == 'FH885XV500':
        config_h.write('#define MAX_VPU_CHN_NUM 4\n')
        config_h.write('#define MAX_GRP_NUM 2\n')
    if chipname == 'FH8626V300':
        config_h.write('#define MAX_VPU_CHN_NUM 3\n')
        config_h.write('#define MAX_GRP_NUM 2\n')

    config = open(config_filename, 'r')

    empty_line = 1

    for line in config:
        line = line.lstrip(' ').replace('\n', '').replace('\r', '')

        if len(line) == 0:
            continue

        if line[0] == '#':
            if len(line) == 1:
                if empty_line:
                    continue

                config_h.write('\n')
                empty_line = 1
                continue

            line = line[2:]

            config_h.write('/*%s */\n' % line)
            empty_line = 0

        else:
            empty_line = 0
            setting = line.split('=')
            if len(setting) >= 2:
                if setting[1] == 'y':
                    config_h.write('#define %s\n' % setting[0])
                    sensor_name = write_sensor(setting[0])
                    if sensor_name:
                        config_h.write('#define %s\n'% sensor_name)

                    result = write_sensor_format(setting[0])
                    if result:
                        isp_format, w, h = result
                        config_h.write('#define %s\n'% isp_format)
                        config_h.write('#define %s\n'% w)
                        config_h.write('#define %s\n'% h)
                else:
                    config_h.write('#define %s %s\n' % (setting[0], setting[1]))

    config_h.write('\n')
    config_h.write('#endif\n')
    config_h.close()

if __name__ == '__main__':

    if len(sys.argv) < 2:
        sys.exit(1)

    config_filename = sys.argv[1]
    filename = sys.argv[2]
    chipname = sys.argv[3]

    write_config(config_filename, filename, chipname)
