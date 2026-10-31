#ifndef __fh_tele_ppp_h__
#define __fh_tele_ppp_h__

/*
 * @brief get PPP status change (up, down, …) from lwIP.
 */
extern int fh_tele_ppp_status(void);
/*
 * @brief start pppos dialing and connect network.
 */
extern int fh_tele_ppp_start(void);

/*
 * @brief stop and disconnect network.
 */
extern int fh_tele_ppp_stop(void);

#endif /* __fh_tele_ppp_h__ */
