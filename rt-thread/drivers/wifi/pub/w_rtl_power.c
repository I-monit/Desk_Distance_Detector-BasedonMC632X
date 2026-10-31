#include <rtthread.h>
#include "api_wifi/generalWifi.h"
#ifdef WIFI_RTL_POWER_CTL

static unsigned char *tx_power_rate =
"#[v1][Exact]#\n"
"#[2.4G][A]#\n"
"[1Tx] 0xe08    0x0000ff00         0 0 16 0\n"
"[1Tx] 0x86c    0xffffff00         16 16 16  0\n"
"[1Tx] 0xe00    0xffffffff         17 18 18 18\n"
"[1Tx] 0xe04    0xffffffff         14 15 16 17\n"
"[1Tx] 0xe10    0xffffffff         15 17 17 17\n"
"[1Tx] 0xe14    0xffffffff         13 13 14 15\n"
"#[END]#\n"
"0xffff 0xffff\n";



static unsigned char *txpwr_lmt =
"## 2.4G, 20M, 1T, CCK,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"
"CH01 16 13 16\n"
"CH02 16 13 16\n"
"CH03 16 13 16\n"
"CH04 16 13 16\n"
"CH05 16 13 16\n"
"CH06 16 13 16\n"
"CH07 16 13 16\n"
"CH08 16 13 16\n"
"CH09 16 13 16\n"
"CH10 16 13 16\n"
"CH11 16 13 16\n"
"CH12 15 13 16\n"
"CH13 13 13 16\n"
"CH14 NA NA 16\n"
"## END\n"

"## 2.4G, 20M, 1T, OFDM,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"
"CH01 14 15 15\n"
"CH02 14 15 15\n"
"CH03 15 15 15\n"
"CH04 15 15 15\n"
"CH05 15 15 15\n"
"CH06 15 15 15\n"
"CH07 15 15 15\n"
"CH08 15 15 15\n"
"CH09 14 15 15\n"
"CH10 14 15 15\n"
"CH11 14 15 15\n"
"CH12 12 15 15\n"
"CH13  8 15 15\n"
"CH14 NA NA NA\n"
"## END\n"

"## 2.4G, 20M, 1T, HT,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"
"CH01 14 15 15\n"
"CH02 14 15 15\n"
"CH03 15 15 15\n"
"CH04 15 15 15\n"
"CH05 15 15 15\n"
"CH06 15 15 15\n"
"CH07 15 15 15\n"
"CH08 15 15 15\n"
"CH09 14 15 15\n"
"CH10 14 15 15\n"
"CH11 14 15 15\n"
"CH12 12 15 15\n"
"CH13  8 15 15\n"
"CH14 NA NA NA\n"
"## END\n"

"## 2.4G, 20M, 2T, HT,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"
"CH01 14 15 15\n"
"CH02 14 15 15\n"
"CH03 15 15 15\n"
"CH04 15 15 15\n"
"CH05 15 15 15\n"
"CH06 15 15 15\n"
"CH07 15 15 15\n"
"CH08 15 15 15\n"
"CH09 14 15 15\n"
"CH10 14 15 15\n"
"CH11 14 15 15\n"
"CH12 NA 15 15\n"
"CH13 NA 15 15\n"
"CH14 NA NA NA\n"
"## END\n"

"## 2.4G, 40M, 1T, HT,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"
"CH01 NA NA NA\n"
"CH02 NA NA NA\n"
"CH03 13 13 13\n"
"CH04 13 13 13\n"
"CH05 13 13 13\n"
"CH06 13 13 13\n"
"CH07 13 13 13\n"
"CH08 13 13 13\n"
"CH09 13 13 13\n"
"CH10 12 13 13\n"
"CH11  5 13 13\n"
"CH12 NA 13 13\n"
"CH13 NA 13 13\n"
"CH14 NA NA NA\n"
"## END\n"

"## 2.4G, 40M, 2T, HT,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"
"CH01 NA NA NA\n"
"CH02 NA NA NA\n"
"CH03 13 13 13\n"
"CH04 13 13 13\n"
"CH05 13 13 13\n"
"CH06 13 13 13\n"
"CH07 13 13 13\n"
"CH08 13 13 13\n"
"CH09 13 13 13\n"
"CH10 13 13 13\n"
"CH11 13 13 13\n"
"CH12 NA 13 13\n"
"CH13 NA 13 13\n"
"CH14 NA NA NA\n"
"## END\n"

"## 5G, 20M, 1T, OFDM,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"

"CH36 15 16 16\n"
"CH40 15 16 16\n"
"CH44 15 16 16\n"
"CH48 15 16 16\n"

"CH52 17 16 16\n"
"CH56 17 16 16\n"
"CH60 16 16 16\n"
"CH64 14 16 16\n"

"CH100 15 16 16\n"
"CH114 15 16 16\n"
"CH108 16 16 16\n"
"CH112 17 16 16\n"
"CH116 17 16 16\n"
"CH120 17 16 16\n"
"CH124 17 16 16\n"
"CH128 16 16 16\n"
"CH132 15 16 16\n"
"CH136 15 16 16\n"
"CH140 14 16 16\n"

"CH149 17 16 NA\n"
"CH153 17 16 NA\n"
"CH157 17 16 NA\n"
"CH161 17 16 NA\n"
"CH165 17 16 NA\n"
"## END\n"

"## 5G, 20M, 1T, HT,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"

"CH36 15 16 16\n"
"CH40 15 16 16\n"
"CH44 15 16 16\n"
"CH48 15 16 16\n"

"CH52 17 16 16\n"
"CH56 17 16 16\n"
"CH60 16 16 16\n"
"CH64 14 16 16\n"

"CH100 15 16 16\n"
"CH114 15 16 16\n"
"CH108 16 16 16\n"
"CH112 17 16 16\n"
"CH116 17 16 16\n"
"CH120 17 16 16\n"
"CH124 17 16 16\n"
"CH128 16 16 16\n"
"CH132 15 16 16\n"
"CH136 15 16 16\n"
"CH140 14 16 16\n"

"CH149 17 16 NA\n"
"CH153 17 16 NA\n"
"CH157 17 16 NA\n"
"CH161 17 16 NA\n"
"CH165 17 16 NA\n"
"## END\n"

"## 5G, 20M, 2T, HT,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"

"CH36 14 15 15\n"
"CH40 14 15 15\n"
"CH44 14 15 15\n"
"CH48 14 15 15\n"

"CH52 17 15 15\n"
"CH56 16 15 15\n"
"CH60 15 15 15\n"
"CH64 13 15 15\n"

"CH100 14 15 15\n"
"CH114 14 15 15\n"
"CH108 15 15 15\n"
"CH112 16 15 15\n"
"CH116 16 15 15\n"
"CH120 17 15 15\n"
"CH124 16 15 15\n"
"CH128 15 15 15\n"
"CH132 14 15 15\n"
"CH136 14 15 15\n"
"CH140 13 15 15\n"

"CH149 17 15 NA\n"
"CH153 17 15 NA\n"
"CH157 17 15 NA\n"
"CH161 17 15 NA\n"
"CH165 17 15 NA\n"
"## END\n"

"## 5G, 40M, 1T, HT,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"

"CH38 15 16 16\n"
"CH46 15 16 16\n"

"CH54 16 16 16\n"
"CH62 16 16 16\n"

"CH102 14 16 16\n"
"CH110 16 16 16\n"
"CH118 17 16 16\n"
"CH126 17 16 16\n"
"CH134 16 16 16\n"

"CH151 17 16 NA\n"
"CH159 17 16 NA\n"
"## END\n"

"## 5G;, 40M, 2T, HT,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"

"CH38 14 15 15\n"
"CH46 14 15 15\n"

"CH54 15 15 15\n"
"CH62 15 15 15\n"

"CH102 13 15 15\n"
"CH110 15 15 15\n"
"CH118 17 15 15\n"
"CH126 16 15 15\n"
"CH134 15 15 15\n"

"CH151 17 15 NA\n"
"CH159 17 15 NA\n"
"## END\n"

"## 5G, 80M, 1T, VHT,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"

"CH42 15 16 16\n"

"CH58 14 16 16\n"

"CH106 15 16 16\n"
"CH122 17 16 16\n"

"CH155 17 16 NA\n"
"## END\n"

"## 5G, 80M, 2T, VHT,\n"
"## START\n"
"## #3# FCC ETSI MKK\n"

"CH42 14 15 15\n"

"CH58 13 15 15\n"

"CH106 14 15 15\n"
"CH122 16 15 15\n"

"CH155 17 15 NA\n"
"## END\n";

static int set_tx_power_rate(unsigned char *buf, int buf_len)
{
    int len = 0;

    rt_kprintf("#####[%s-%d]#####\n", __func__, __LINE__);

    len = rt_strlen((const char *)tx_power_rate);
    if (len > buf_len)
    {
        rt_kprintf("tx power rate length is too long. \n");
        return 0;
    }
    rt_memcpy(buf, tx_power_rate, len);

    return len;
}


static int set_tx_power_limit(unsigned char *buf, int buf_len)
{
    int len = 0;

    rt_kprintf("#####[%s-%d]#####\n", __func__, __LINE__);

    len = rt_strlen((const char *)txpwr_lmt);
    if (len > buf_len)
    {
        rt_kprintf("tx power limit length is too long. \n");
        return 0;
    }
    rt_memcpy(buf, txpwr_lmt, len);

    return len;
}

void rtl_power_config(void)
{
    w_power_rate_set_cb(set_tx_power_rate);

    w_power_limit_set_cb(set_tx_power_limit);
}
#else

void rtl_power_config(void)
{
    ;
}
#endif
