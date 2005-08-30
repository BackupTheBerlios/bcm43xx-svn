#ifndef BCM430X_MICROCODE_H_
#define BCM430X_MICROCODE_H_

#include <linux/types.h>

extern const u32 bcm430x_ucode2_data[];
extern const u32 bcm430x_ucode4_data[];
extern const u32 bcm430x_ucode5_data[];
extern const u32 bcm430x_pcm4_data[];
extern const u32 bcm430x_pcm5_data[];

/* sizes in dwords (u32, not bytes) */
extern const unsigned int bcm430x_ucode2_size;
extern const unsigned int bcm430x_ucode4_size;
extern const unsigned int bcm430x_ucode5_size;
extern const unsigned int bcm430x_pcm4_size;
extern const unsigned int bcm430x_pcm5_size;

#endif /* BCM430X_MICROCODE_H_ */
