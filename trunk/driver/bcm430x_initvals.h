#ifndef BCM430x_INITVALS_H_
#define BCM430x_INITVALS_H_

#include <linux/types.h>


struct bcm430x_initval {
	u16 offset;
	u8 size;
	u32 value;
};

extern const struct bcm430x_initval bcm430x_initvals_core24_aphy[];
extern const struct bcm430x_initval bcm430x_initvals_core24_bgphy[];
extern const struct bcm430x_initval bcm430x_initvals_core5_aphy[];
extern const struct bcm430x_initval bcm430x_bsinitvals_core5_aphy_1[];
extern const struct bcm430x_initval bcm430x_bsinitvals_core5_aphy_2[];
extern const struct bcm430x_initval bcm430x_initvals_core5_bgphy[];
extern const struct bcm430x_initval bcm430x_bsinitvals_core5_bgphy[];

#endif /* BCM430x_INITVALS_H_ */

/* vim: set ts=8 sw=8 sts=8: */
