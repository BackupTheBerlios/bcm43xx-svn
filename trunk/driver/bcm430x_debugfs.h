#ifndef BCM430x_DEBUGFS_H_
#define BCM430x_DEBUGFS_H_

#ifdef BCM430x_DEBUG

void bcm430x_debugfs_init(void);
void bcm430x_debugfs_exit(void);

#else /* BCM430x_DEBUG */

static inline
void bcm430x_debugfs_init()
{
}

static inline
void bcm430x_debugfs_exit()
{
}

#endif /* BCM430x_DEBUG */

#endif /* BCM430x_DEBUGFS_H_ */
