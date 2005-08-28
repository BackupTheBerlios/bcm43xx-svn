#ifndef BCM430x_DEBUGFS_H_
#define BCM430x_DEBUGFS_H_

#ifdef BCM430x_DEBUG

#include <linux/list.h>
#include <asm/semaphore.h>

struct bcm430x_private;
struct dentry;

struct bcm430x_dfsentry
{
	struct dentry *subdir;
	struct dentry *dentry_devinfo;
	struct dentry *dentry_spromdump;
	struct dentry *dentry_shmdump;

	struct bcm430x_private *bcm;
	struct list_head list;
};

struct bcm430x_debugfs
{
	struct semaphore sem;
	struct dentry *root;
	struct dentry *dentry_driverinfo;
	struct list_head entries;
	int nr_entries;
};

void bcm430x_debugfs_init(void);
void bcm430x_debugfs_exit(void);
void bcm430x_debugfs_add_device(struct bcm430x_private *bcm);
void bcm430x_debugfs_remove_device(struct bcm430x_private *bcm);

#else /* BCM430x_DEBUG */

static inline
void bcm430x_debugfs_init(void) { }
static inline
void bcm430x_debugfs_exit(void) { }
static inline
void bcm430x_debugfs_add_device(struct bcm430x_private *bcm) { }
static inline
void bcm430x_debugfs_remove_device(struct bcm430x_private *bcm) { }

#endif /* BCM430x_DEBUG */

#endif /* BCM430x_DEBUGFS_H_ */
