#ifndef _GPS_DL_TIME_TICK_H
#define _GPS_DL_TIME_TICK_H

#define GPS_DL_RW_NO_TIMEOUT (-1)
void gps_dl_wait_us(unsigned int us);
#define GDL_WAIT_US(Usec) gps_dl_wait_us(Usec)
unsigned int gps_dl_tick_get(void);
int gps_dl_tick_delta_to_usec(unsigned int tick0, unsigned int tick1);

#endif /* _GPS_DL_TIME_TICK_H */

