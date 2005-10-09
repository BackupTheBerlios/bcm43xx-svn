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
	struct dentry *dentry_tsf;

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

/* Debug helper: Dump binary data through printk. */
void bcm430x_printk_dump(const char *data,
			 size_t size,
			 const char *description);
/* Debug helper: Dump bitwise binary data through printk. */
void bcm430x_printk_bitdump(const unsigned char *data,
			    size_t bytes,
			    const char *description);
#define bcm430x_printk_bitdumpt(pointer, description) \
	do {									\
		bcm430x_printk_bitdump((const unsigned char *)(pointer),	\
				       sizeof(*(pointer)),			\
				       (description));				\
	} while (0)

#else /* BCM430x_DEBUG */

static inline
void bcm430x_debugfs_init(void) { }
static inline
void bcm430x_debugfs_exit(void) { }
static inline
void bcm430x_debugfs_add_device(struct bcm430x_private *bcm) { }
static inline
void bcm430x_debugfs_remove_device(struct bcm430x_private *bcm) { }

static inline
void bcm430x_printk_dump(const char *data,
			 size_t size,
			 const char *description)
{
}

#endif /* BCM430x_DEBUG */

/* Ugly helper macros to make incomplete code more verbose on runtime */
#ifdef TODO
# undef TODO
#endif
#define TODO()  \
	do {										\
		printk(KERN_INFO PFX "TODO: Incomplete code in %s() at %s:%d\n",	\
		       __FUNCTION__, __FILE__, __LINE__);				\
	} while (0)

#ifdef FIXME
# undef FIXME
#endif
#define FIXME()  \
	do {										\
		printk(KERN_INFO PFX "FIXME: Possibly broken code in %s() at %s:%d\n",	\
		       __FUNCTION__, __FILE__, __LINE__);				\
	} while (0)

#endif /* BCM430x_DEBUGFS_H_ */
