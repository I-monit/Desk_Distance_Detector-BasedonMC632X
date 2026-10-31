#ifndef _FH_CRC32_H_
#define _FH_CRC32_H_

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int fh_crc32(unsigned int crc32val, unsigned char *s, int len);
extern unsigned int fh_crc32_ch(unsigned int crc32val, unsigned char ch, int len);
extern unsigned int fh_ether_crc32(unsigned int crc32val, unsigned char *s, int len);
extern unsigned int fh_ether_crc32_ch(unsigned int crc32val, unsigned char ch, int len);

#ifdef __cplusplus
}
#endif

#endif // _FH_CRC32_H_
// E.O.F
