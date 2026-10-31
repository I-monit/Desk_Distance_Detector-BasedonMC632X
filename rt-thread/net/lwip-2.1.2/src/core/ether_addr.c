#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>

struct ether_addr *lwip_ether_aton_r(const char *asc, struct ether_addr *addr)
{
    /* asc is "X:XX:XX:x:xx:xX" */
    int cnt;

    for (cnt = 0; cnt < 6; ++cnt) {
        unsigned char number;
        char ch = *asc++;

        if (ch < 0x20)
            return NULL;
        /* | 0x20 is cheap tolower(), valid for letters/numbers only */
        ch |= 0x20;
        if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
            return NULL;
        number = !(ch > '9') ? (ch - '0') : (ch - 'a' + 10);

        ch = *asc++;
        if ((cnt != 5 && ch != ':') /* not last group */
        /* What standard says ASCII ether address representation
         * may also finish with whitespace, not only NUL?
         * We can get rid of isspace() otherwise */
         || (cnt == 5 && ch != '\0' /*&& !isspace(ch)*/)
        ) {
            ch |= 0x20; /* cheap tolower() */
            if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
                return NULL;
            number = (number << 4) + (!(ch > '9') ? (ch - '0') : (ch - 'a' + 10));

            if (cnt != 5) {
                ch = *asc++;
                if (ch != ':')
                    return NULL;
            }
        }

        /* Store result.  */
        addr->ether_addr_octet[cnt] = number;
    }
    /* Looks like we allow garbage after last group?
     * "1:2:3:4:5:66anything_at_all"? */

    return addr;
}

char *lwip_ether_ntoa_r(const struct ether_addr *addr, char *buf)
{
    sprintf(buf, "%x:%x:%x:%x:%x:%x",
            addr->ether_addr_octet[0], addr->ether_addr_octet[1],
            addr->ether_addr_octet[2], addr->ether_addr_octet[3],
            addr->ether_addr_octet[4], addr->ether_addr_octet[5]);
    return buf;
}
