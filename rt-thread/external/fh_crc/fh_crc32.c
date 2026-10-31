#include "fh_crc32.h"

static const unsigned int _g_fh_crc32_table[256] =
{
	0x00000000,		// 00
	0x77073096,		// 01
	0xEE0E612C,		// 02
	0x990951BA,		// 03
	0x076DC419,		// 04
	0x706AF48F,		// 05
	0xE963A535,		// 06
	0x9E6495A3,		// 07
	0x0EDB8832,		// 08
	0x79DCB8A4,		// 09
	0xE0D5E91E,		// 0A
	0x97D2D988,		// 0B
	0x09B64C2B,		// 0C
	0x7EB17CBD,		// 0D
	0xE7B82D07,		// 0E
	0x90BF1D91,		// 0F
	0x1DB71064,		// 10
	0x6AB020F2,		// 11
	0xF3B97148,		// 12
	0x84BE41DE,		// 13
	0x1ADAD47D,		// 14
	0x6DDDE4EB,		// 15
	0xF4D4B551,		// 16
	0x83D385C7,		// 17
	0x136C9856,		// 18
	0x646BA8C0,		// 19
	0xFD62F97A,		// 1A
	0x8A65C9EC,		// 1B
	0x14015C4F,		// 1C
	0x63066CD9,		// 1D
	0xFA0F3D63,		// 1E
	0x8D080DF5,		// 1F
	0x3B6E20C8,		// 20
	0x4C69105E,		// 21
	0xD56041E4,		// 22
	0xA2677172,		// 23
	0x3C03E4D1,		// 24
	0x4B04D447,		// 25
	0xD20D85FD,		// 26
	0xA50AB56B,		// 27
	0x35B5A8FA,		// 28
	0x42B2986C,		// 29
	0xDBBBC9D6,		// 2A
	0xACBCF940,		// 2B
	0x32D86CE3,		// 2C
	0x45DF5C75,		// 2D
	0xDCD60DCF,		// 2E
	0xABD13D59,		// 2F
	0x26D930AC,		// 30
	0x51DE003A,		// 31
	0xC8D75180,		// 32
	0xBFD06116,		// 33
	0x21B4F4B5,		// 34
	0x56B3C423,		// 35
	0xCFBA9599,		// 36
	0xB8BDA50F,		// 37
	0x2802B89E,		// 38
	0x5F058808,		// 39
	0xC60CD9B2,		// 3A
	0xB10BE924,		// 3B
	0x2F6F7C87,		// 3C
	0x58684C11,		// 3D
	0xC1611DAB,		// 3E
	0xB6662D3D,		// 3F
	0x76DC4190,		// 40
	0x01DB7106,		// 41
	0x98D220BC,		// 42
	0xEFD5102A,		// 43
	0x71B18589,		// 44
	0x06B6B51F,		// 45
	0x9FBFE4A5,		// 46
	0xE8B8D433,		// 47
	0x7807C9A2,		// 48
	0x0F00F934,		// 49
	0x9609A88E,		// 4A
	0xE10E9818,		// 4B
	0x7F6A0DBB,		// 4C
	0x086D3D2D,		// 4D
	0x91646C97,		// 4E
	0xE6635C01,		// 4F
	0x6B6B51F4,		// 50
	0x1C6C6162,		// 51
	0x856530D8,		// 52
	0xF262004E,		// 53
	0x6C0695ED,		// 54
	0x1B01A57B,		// 55
	0x8208F4C1,		// 56
	0xF50FC457,		// 57
	0x65B0D9C6,		// 58
	0x12B7E950,		// 59
	0x8BBEB8EA,		// 5A
	0xFCB9887C,		// 5B
	0x62DD1DDF,		// 5C
	0x15DA2D49,		// 5D
	0x8CD37CF3,		// 5E
	0xFBD44C65,		// 5F
	0x4DB26158,		// 60
	0x3AB551CE,		// 61
	0xA3BC0074,		// 62
	0xD4BB30E2,		// 63
	0x4ADFA541,		// 64
	0x3DD895D7,		// 65
	0xA4D1C46D,		// 66
	0xD3D6F4FB,		// 67
	0x4369E96A,		// 68
	0x346ED9FC,		// 69
	0xAD678846,		// 6A
	0xDA60B8D0,		// 6B
	0x44042D73,		// 6C
	0x33031DE5,		// 6D
	0xAA0A4C5F,		// 6E
	0xDD0D7CC9,		// 6F
	0x5005713C,		// 70
	0x270241AA,		// 71
	0xBE0B1010,		// 72
	0xC90C2086,		// 73
	0x5768B525,		// 74
	0x206F85B3,		// 75
	0xB966D409,		// 76
	0xCE61E49F,		// 77
	0x5EDEF90E,		// 78
	0x29D9C998,		// 79
	0xB0D09822,		// 7A
	0xC7D7A8B4,		// 7B
	0x59B33D17,		// 7C
	0x2EB40D81,		// 7D
	0xB7BD5C3B,		// 7E
	0xC0BA6CAD,		// 7F
	0xEDB88320,		// 80
	0x9ABFB3B6,		// 81
	0x03B6E20C,		// 82
	0x74B1D29A,		// 83
	0xEAD54739,		// 84
	0x9DD277AF,		// 85
	0x04DB2615,		// 86
	0x73DC1683,		// 87
	0xE3630B12,		// 88
	0x94643B84,		// 89
	0x0D6D6A3E,		// 8A
	0x7A6A5AA8,		// 8B
	0xE40ECF0B,		// 8C
	0x9309FF9D,		// 8D
	0x0A00AE27,		// 8E
	0x7D079EB1,		// 8F
	0xF00F9344,		// 90
	0x8708A3D2,		// 91
	0x1E01F268,		// 92
	0x6906C2FE,		// 93
	0xF762575D,		// 94
	0x806567CB,		// 95
	0x196C3671,		// 96
	0x6E6B06E7,		// 97
	0xFED41B76,		// 98
	0x89D32BE0,		// 99
	0x10DA7A5A,		// 9A
	0x67DD4ACC,		// 9B
	0xF9B9DF6F,		// 9C
	0x8EBEEFF9,		// 9D
	0x17B7BE43,		// 9E
	0x60B08ED5,		// 9F
	0xD6D6A3E8,		// A0
	0xA1D1937E,		// A1
	0x38D8C2C4,		// A2
	0x4FDFF252,		// A3
	0xD1BB67F1,		// A4
	0xA6BC5767,		// A5
	0x3FB506DD,		// A6
	0x48B2364B,		// A7
	0xD80D2BDA,		// A8
	0xAF0A1B4C,		// A9
	0x36034AF6,		// AA
	0x41047A60,		// AB
	0xDF60EFC3,		// AC
	0xA867DF55,		// AD
	0x316E8EEF,		// AE
	0x4669BE79,		// AF
	0xCB61B38C,		// B0
	0xBC66831A,		// B1
	0x256FD2A0,		// B2
	0x5268E236,		// B3
	0xCC0C7795,		// B4
	0xBB0B4703,		// B5
	0x220216B9,		// B6
	0x5505262F,		// B7
	0xC5BA3BBE,		// B8
	0xB2BD0B28,		// B9
	0x2BB45A92,		// BA
	0x5CB36A04,		// BB
	0xC2D7FFA7,		// BC
	0xB5D0CF31,		// BD
	0x2CD99E8B,		// BE
	0x5BDEAE1D,		// BF
	0x9B64C2B0,		// C0
	0xEC63F226,		// C1
	0x756AA39C,		// C2
	0x026D930A,		// C3
	0x9C0906A9,		// C4
	0xEB0E363F,		// C5
	0x72076785,		// C6
	0x05005713,		// C7
	0x95BF4A82,		// C8
	0xE2B87A14,		// C9
	0x7BB12BAE,		// CA
	0x0CB61B38,		// CB
	0x92D28E9B,		// CC
	0xE5D5BE0D,		// CD
	0x7CDCEFB7,		// CE
	0x0BDBDF21,		// CF
	0x86D3D2D4,		// D0
	0xF1D4E242,		// D1
	0x68DDB3F8,		// D2
	0x1FDA836E,		// D3
	0x81BE16CD,		// D4
	0xF6B9265B,		// D5
	0x6FB077E1,		// D6
	0x18B74777,		// D7
	0x88085AE6,		// D8
	0xFF0F6A70,		// D9
	0x66063BCA,		// DA
	0x11010B5C,		// DB
	0x8F659EFF,		// DC
	0xF862AE69,		// DD
	0x616BFFD3,		// DE
	0x166CCF45,		// DF
	0xA00AE278,		// E0
	0xD70DD2EE,		// E1
	0x4E048354,		// E2
	0x3903B3C2,		// E3
	0xA7672661,		// E4
	0xD06016F7,		// E5
	0x4969474D,		// E6
	0x3E6E77DB,		// E7
	0xAED16A4A,		// E8
	0xD9D65ADC,		// E9
	0x40DF0B66,		// EA
	0x37D83BF0,		// EB
	0xA9BCAE53,		// EC
	0xDEBB9EC5,		// ED
	0x47B2CF7F,		// EE
	0x30B5FFE9,		// EF
	0xBDBDF21C,		// F0
	0xCABAC28A,		// F1
	0x53B39330,		// F2
	0x24B4A3A6,		// F3
	0xBAD03605,		// F4
	0xCDD70693,		// F5
	0x54DE5729,		// F6
	0x23D967BF,		// F7
	0xB3667A2E,		// F8
	0xC4614AB8,		// F9
	0x5D681B02,		// FA
	0x2A6F2B94,		// FB
	0xB40BBE37,		// FC
	0xC30C8EA1,		// FD
	0x5A05DF1B,		// FE
	0x2D02EF8D		// FF
};

unsigned int fh_crc32(unsigned int crc32val, unsigned char *s, int len)
{
    int i;

    for (i = 0;  i < len;  i++)
    {
        crc32val = _g_fh_crc32_table[(crc32val ^ s[i]) & 0xff] ^ (crc32val >> 8);
    }
    return crc32val;
}

unsigned int fh_crc32_ch(unsigned int crc32val, unsigned char ch, int len)
{
    int i;

    for (i = 0;  i < len;  i++)
    {
        crc32val = _g_fh_crc32_table[(crc32val ^ ch) & 0xff] ^ (crc32val >> 8);
    }
    return crc32val;
}

unsigned int fh_ether_crc32(unsigned int crc32val, unsigned char *s, int len)
{
    int i;

    if(!s)
        return 0L;

    crc32val = crc32val ^ 0xffffffff;
    for (i = 0;  i < len;  i++)
    {
        crc32val = _g_fh_crc32_table[(crc32val ^ s[i]) & 0xff] ^ (crc32val >> 8);
    }
    return crc32val ^ 0xffffffff;
}

unsigned int fh_ether_crc32_ch(unsigned int crc32val, unsigned char ch, int len)
{
    int i;

    crc32val = crc32val ^ 0xffffffff;
    for (i = 0;  i < len;  i++)
    {
        crc32val = _g_fh_crc32_table[(crc32val ^ ch) & 0xff] ^ (crc32val >> 8);
    }
    return crc32val ^ 0xffffffff;
}

// E.O.F
