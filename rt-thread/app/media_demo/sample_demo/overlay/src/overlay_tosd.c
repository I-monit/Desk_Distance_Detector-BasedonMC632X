#include "overlay/include/overlay_tosd.h"
#include "overlay/include/font_array.h"

static char *model_name = MODEL_TAG_DEMO_OVERLAY_TOSD;

static FH_UINT32 g_tosd_inited[MAX_GRP_NUM] = {0};

static FH_UINT8 user_tag_codes[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'k', 'b', 'p', 's' /* 0 - 9, kpbs*/
};                                                                       /* 设置FHT_OSD_USER1对应的字符集合 */

/* 把8位的ascii字符转换为32位 */
static FH_VOID stringformchartoint(FH_UINT32 *code, FH_CHAR *s, FH_UINT32 cnt)
{
    FH_SINT32 i;

    for (i = 0; i < cnt; i++)
    {
        code[i] = (FH_UINT32)((FH_UINT8)(s[i]));
    }
}

/* 自定义字符回调函数，自动刷新FHT_OSD_USER1对应的字符内容，显示实时码率，调用频率和时间刷新频率相同 */
static FH_UINT32 sample_osd_char_refresh_callback(FH_UINT32 tagCode, FHT_DATE_TIME *time, FH_UINT32 *pCodeArray)
{
    FH_SINT32 ret;
    FH_UINT32 cnt;
    FH_SINT32 chn_id;
    FH_CHAR data[FHT_OSD_MAX_CHARS_OF_TAG];
    FH_UINT32 bps; // 实际码率 | [ ]

    chn_id = tagCode - FHT_OSD_USER1; /*每个通道使用不同的TAG,chn_id*/

    ret = getENCStreamBsp(FH_APP_GRP_ID, chn_id, &bps); // TODO 无法获取准确的chn bsp
    if (ret != 0)
    {
        bps = 0;
    }
    cnt = snprintf(data, sizeof(data), "%7d kbps", bps / 1000);
    stringformchartoint(pCodeArray, data, cnt);

    /*
     * 注意: 1) 每次调用该函数的返回值必须保持一致,并且不能大于FHT_OSD_MAX_CHARS_OF_TAG,
     *       2) 最多向数组pCodeArray中写入FHT_OSD_MAX_CHARS_OF_TAG个FH_UINT32的值,否则会出现堆栈溢出等问题
     */
    return cnt;
}

/* USER_TAG回调函数，作用为告诉ADVAPI库，自定义字符会用到的所有字符*/
static FH_UINT32 sample_osd_char_traverse_callback(FH_UINT32 tag_code, FH_UINT32 sequence)
{

    if (sequence >= sizeof(user_tag_codes))
        return 0; /*如果sequence大于等于集合字符数,表示枚举结束,返回0*/

    return (FH_UINT32)(user_tag_codes[sequence]); /* 返回字符集合中第sequence个字符的编码 */
}

/* 获取汉字在字库中的地址偏移 */
static FH_UINT32 GET_HZ_OFFSET(FH_UINT32 x)
{
    // GB2312
    return ((94 * ((x >> 8) - 0xa0 - 1) + ((x & 0xff) - 0xa0 - 1)) * 32);
}

/* 自定义字库回调函数，主要功能用于返回字符code对应的点阵地址及点阵的宽高 */
/* 注意: 返回的字符点阵地址最好为静态地址，保证在advapi拷贝完整正确的点阵数据前，这一地址上的数据不被修改 */
/* advapi内部会缓存所有设置过的字符点阵，如果确认后续不会再使用的新的字符，可以在设置结束后释放字库资源 */
FH_UINT8 *get_font_addr(FH_UINT32 code, FH_UINT32 *pWidth, FH_UINT32 *pHeight)
{
    if (code < 0xff)
    {
        *pWidth = 8;              /* 该自定义字库中每个asc字符的点阵宽度为8个像素点 */
        *pHeight = 16;            /* 该自定义字库中每个asc字符的点阵高度为16个像素点 */
        return asc16 + 16 * code; /* 返回字符code对应的点阵存储地址 */
    }
    else
    {
        *pWidth = 16;                        /* 该自定义字库中每个中文字符的点阵宽度为16个像素点 */
        *pHeight = 16;                       /* 该自定义字库中每个中文字符的点阵高度为16个像素点 */
        return gb2312 + GET_HZ_OFFSET(code); /* 返回字符code对应的点阵存储地址 */
    }
}

/* 加载字库 */
static FH_SINT32 load_font_lib(FHT_OSD_FontType_e type, FH_UINT8 *font_array, FH_SINT32 font_array_size)
{
    FH_SINT32 ret;
    FHT_OSD_FontLib_t font_lib;

    font_lib.pLibData = font_array;
    font_lib.libSize = font_array_size;
    ret = FHAdv_Osd_LoadFontLib(type, &font_lib);
    if (ret != 0)
    {
        printf("Error: FHAdv_Osd_LoadFontLib failed, ret=%d\n", ret);
        return ret;
    }

    return 0;
}

static FH_SINT32 sample_set_textline_use_time_format(FH_SINT32 channel, FHT_OSD_TextLine_t *text_line)
{
    FH_CHAR user_tag_data[] = {
        0xe4, 0x0d + channel, /*FHT_OSD_USER1,这里用来显示码率*/
        0x0a,                 /*换行*/
        0,                    /*null terminated string*/
    };

    sprintf(text_line->textInfo, "Channel-%d ", channel);
    strcat(text_line->textInfo, user_tag_data);
    text_line->textEnable = 1;    /* 使能自定义text */
    text_line->timeOsdEnable = 1; /* 使能时间显示 */

    /* 12小时制式、显示星期、时间在自定义字符后面显示 */
    text_line->timeOsdFlags = FHT_OSD_TIME_BIT_HOUR12 | FHT_OSD_TIME_BIT_WEEK | FHT_OSD_TIME_BIT_AFTER;

    text_line->timeOsdFormat = FHTEN_OSD_TimeFmt0; /* 使用时间格式FHTEN_OSD_TimeFmt0  YYYY-MM-DD 00:00:00  */

    text_line->textLineWidth = (OSD_FONT_DISP_SIZE / 2) * 36; /* 每行最多显示36个像素宽度为16的字符, 超过后自动换行 */
    text_line->linePositionX = 0;                             /* 左上角起始点水平方向像素位置 */
    text_line->linePositionY = 0;                             /* 左上角起始点垂直方向像素位置 */
    text_line->lineId = 0;
    text_line->enable = 1;
    return 0;
}

static FH_SINT32 sample_set_textline_use_user_tag(FH_SINT32 channel, FHT_OSD_TextLine_t *text_line)
{
    FH_CHAR user_tag_data[] = {
        0xe4, 0x0d + channel, /*FHT_OSD_USER1,此处用于显示当前码率*/
        0x0a,                 /*换行*/
        0xe4, 0x01,           /*FHT_OSD_YEAR4, 4位年,比如2019*/
        '-',
        0xe4, 0x03, /*FHT_OSD_MONTH2, 2位月份,取值01～12*/
        '-',
        0xe4, 0x04, /*FHT_OSD_DAY, 2位日期,取值01～31*/
        0x20,       /*空格*/
        0xe4, 0x07, /*FHT_OSD_HOUR24, 24时制小时,取值00～23*/
        ':',
        0xe4, 0x09, /*FHT_OSD_MINUTE, 2位分钟,取值00～59*/
        ':',
        0xe4, 0x0a, /*FHT_OSD_SECOND, 2位秒,取值00～59*/
        0,          /*null terminated string*/
    };

    sprintf(text_line->textInfo, "Channel-%d ", channel);
    strcat(text_line->textInfo, user_tag_data);
    text_line->textEnable = 1;                                /* 使能自定义text */
    text_line->timeOsdEnable = 0;                             /* 去使能时间显示 */
    text_line->textLineWidth = (OSD_FONT_DISP_SIZE / 2) * 36; /* 每行最多显示36个像素宽度为16的字符 */
    text_line->linePositionX = 0;                             /* 左上角起始点宽度像素位置 */
    text_line->linePositionY = 0;                             /* 左上角起始点高度像素位置 */

    text_line->lineId = 0;
    text_line->enable = 1;
    return 0;
}

static FH_SINT32 sample_set_channel_tosd(FH_SINT32 vpuid, FH_SINT32 channel)
{
    FH_SINT32 ret;
    FHT_OSD_CONFIG_t osd_cfg;
    FHT_OSD_Layer_Config_t pOsdLayerInfo[1];
    FHT_OSD_TextLine_t text_line_cfg[1];
    FHT_OSD_Ex_TextLine_t TextCfg[1];
    FH_CHAR text_data[128]; /*it should be enough*/
    FH_SINT32 user_defined_time = 0;

    memset(&osd_cfg, 0, sizeof(osd_cfg));
    memset(&pOsdLayerInfo[0], 0, sizeof(FHT_OSD_Layer_Config_t));
    memset(&text_line_cfg[0], 0, sizeof(FHT_OSD_TextLine_t));
    memset(&TextCfg[0], 0, sizeof(FHT_OSD_Ex_TextLine_t));
    memset(&text_data, 0, sizeof(text_data));

    /* 不旋转 */
    osd_cfg.osdRotate = 0;
    osd_cfg.pOsdLayerInfo = &pOsdLayerInfo[0];
    /* 设置text行块个数 */
    osd_cfg.nOsdLayerNum = 1; /*我们的demo中只演示了一个行块*/

    pOsdLayerInfo[0].layerStartX = 0;
    pOsdLayerInfo[0].layerStartY = 0;
    pOsdLayerInfo[0].layerMaxWidth = 640; /*根据需求配置，如果设置则以设置的最大值配置内存，如果缺省则以实际幅面大小申请内存*/
    pOsdLayerInfo[0].layerMaxHeight = 480;
    /* 设置字符大小,像素单位 */
    pOsdLayerInfo[0].osdSize = OSD_FONT_DISP_SIZE;

    /* 设置字符颜色为白色 */
    pOsdLayerInfo[0].normalColor.fAlpha = 255;
    pOsdLayerInfo[0].normalColor.fRed = 255;
    pOsdLayerInfo[0].normalColor.fGreen = 255;
    pOsdLayerInfo[0].normalColor.fBlue = 255;

    /* 设置字符反色颜色为黑色 */
    pOsdLayerInfo[0].invertColor.fAlpha = 255;
    pOsdLayerInfo[0].invertColor.fRed = 0;
    pOsdLayerInfo[0].invertColor.fGreen = 0;
    pOsdLayerInfo[0].invertColor.fBlue = 0;

    /* 设置字符钩边颜色为黑色 */
    pOsdLayerInfo[0].edgeColor.fAlpha = 255;
    pOsdLayerInfo[0].edgeColor.fRed = 0;
    pOsdLayerInfo[0].edgeColor.fGreen = 0;
    pOsdLayerInfo[0].edgeColor.fBlue = 0;

    /* 不显示背景 */
    pOsdLayerInfo[0].bkgColor.fAlpha = 0;

    /* 钩边像素为1 */
    pOsdLayerInfo[0].edgePixel = 1;

    /* 反色控制 */
    pOsdLayerInfo[0].osdInvertEnable = FH_OSD_INVERT_DISABLE; /*disable反色功能*/
    // pOsdLayerInfo[0].osdInvertEnable  = FH_OSD_INVERT_BY_CHAR; /*以字符为单位进行反色控制*/
    /*osd_cfg.osdInvertEnable  = FH_OSD_INVERT_BY_LINE;*/ /*以行块为单位进行反色控制*/
    pOsdLayerInfo[0].osdInvertThreshold.high_level = 180;
    pOsdLayerInfo[0].osdInvertThreshold.low_level = 160;
    pOsdLayerInfo[0].layerFlag = FHT_OSD_LAYER_DEFAULT;
#ifdef FH_APP_OVERLAY_TOSD_TWOBUF
    pOsdLayerInfo[0].layerFlag = FH_OSD_LAYER_USE_TWO_BUF;
#endif
    pOsdLayerInfo[0].layerId = 0;

    ret = FHAdv_Osd_Ex_SetText(vpuid, channel, &osd_cfg);
    if (ret != FH_SUCCESS)
    {
        if (channel == 0) /*TOSD可能位于分通道前,这样channel 1可能没有,因此就不打印出错*/
        {
            printf("FHAdv_Osd_Ex_SetText failed with %d\n", ret);
        }
        return ret;
    }

    text_line_cfg[0].textInfo = text_data;
    if (!user_defined_time)
    {
        /* 使用ADVAPI定义的时间格式 */
        sample_set_textline_use_time_format(channel, &text_line_cfg[0]);
    }
    else
    {
        /* 使用时间符号设置自定义时间格式*/
        sample_set_textline_use_user_tag(channel, &text_line_cfg[0]);
    }

    TextCfg[0].LineNum = 1;
    TextCfg[0].Reconfig = 1;
    TextCfg[0].pLineCfg = &text_line_cfg[0];
    ret = FHAdv_Osd_Ex_SetTextLine(vpuid, channel, pOsdLayerInfo[0].layerId, &TextCfg[0]);
    if (ret != FH_SUCCESS)
    {
        if (channel == 0) /*TOSD可能位于分通道前,这样channel 1可能没有,因此就不打印出错*/
        {
            printf("FHAdv_Osd_Ex_SetTextLine failed with %d\n", ret);
        }
        return ret;
    }

    return 0;
}

FH_SINT32 sample_set_tosd(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_UINT32 use_ext_font_lib = 0;

    if (!g_tosd_inited[grp_id])
    {
        if (use_ext_font_lib)
        {
            /* 注册自定义字库回调函数，此处设置编码为GB2312, 其他编码方式使用方法相同 */
            ret = FHAdv_Osd_Font_RegisterCallback(get_font_addr, FHTEN_OSD_GB2312);
            SDK_FUNC_ERROR(model_name, ret);
        }
        else
        {
            /* 加载asc字库 */
            ret = load_font_lib(FHEN_FONT_TYPE_ASC, asc16, sizeof(asc16));
            SDK_FUNC_ERROR(model_name, ret);

            /* 加载gb2312字库 */
            ret = load_font_lib(FHEN_FONT_TYPE_CHINESE, gb2312, sizeof(gb2312));
            SDK_FUNC_ERROR(model_name, ret);
        }

        /* 注册FHT_OSD_USER1的回调函数，其他用户自定义字符和FHT_OSD_USER1设置方式相同 */
        /* 第二个参数设置该自定义字符的刷新频率,这里为设置为每秒刷新一次*/
        /* 最多注册16个自定义字符 */
        /* 所有使用到的user TAG要先通过函数FHAdv_Osd_SetTagCallback注册好,否则会报错*/
        ret = FHAdv_Osd_SetTagCallback(grp_id, FHT_OSD_USER1 + 0, FHT_UPDATED_SECOND, sample_osd_char_traverse_callback, sample_osd_char_refresh_callback);
        SDK_FUNC_ERROR(model_name, ret);

        ret = FHAdv_Osd_SetTagCallback(grp_id, FHT_OSD_USER1 + 1, FHT_UPDATED_SECOND, sample_osd_char_traverse_callback, sample_osd_char_refresh_callback);
        SDK_FUNC_ERROR(model_name, ret);

        g_tosd_inited[grp_id] = 1;
    }

#if (SAMPLE_TOSD_GRAPH == FHT_OSD_GRAPH_CTRL_TOSD_BEFORE_VP)
    /*注意: 如果TOSD配置为分通道前，那么只有通道0可以配置，并且其它所有通道的TOSD信息和通道0保持一致*/
    sample_set_channel_tosd(grp_id, 0);
#else
    VPU_CHN_INFO *vpu_chn_info;
    for (FH_SINT32 chn_id = 0; chn_id < MAX_VPU_CHN_NUM - 1; chn_id++) // NN通道不需要TOSD
    {
        vpu_chn_info = get_vpu_chn_config(grp_id, chn_id);
        if (vpu_chn_info->enable)
        {
            sample_set_channel_tosd(grp_id, chn_id);
        }
    }
#endif

    return ret;
}

FH_SINT32 sample_unload_tosd(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (g_tosd_inited[grp_id])
    {
        ret = FHAdv_Osd_UnLoadFontLib(FHEN_FONT_TYPE_ASC);
        SDK_FUNC_ERROR(model_name, ret);

        ret = FHAdv_Osd_UnLoadFontLib(FHEN_FONT_TYPE_CHINESE);
        SDK_FUNC_ERROR(model_name, ret);

        g_tosd_inited[grp_id] = 0;
    }

    return ret;
}
