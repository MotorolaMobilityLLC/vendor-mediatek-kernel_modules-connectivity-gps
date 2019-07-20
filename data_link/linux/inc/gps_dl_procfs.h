#ifndef _GPS_DL_PROCFS_H
#define _GPS_DL_PROCFS_H

int gps_dl_procfs_setup(void);
int gps_dl_procfs_remove(void);

typedef int(*gps_dl_procfs_test_func_type) (int par1, int par2, int par3);

#endif /* _GPS_DL_PROCFS_H */

