#ifndef BCM430x_ILT_H_
#define BCM430x_ILT_H_

#define BCM430x_ILT_ROTOR_SIZE		53
extern const u32 bcm430x_ilt_rotor[BCM430x_ILT_ROTOR_SIZE];
#define BCM430x_ILT_RETARD_SIZE		53
extern const u32 bcm430x_ilt_retard[BCM430x_ILT_RETARD_SIZE];
#define BCM430x_ILT_FINEFREQA_SIZE	256
extern const u16 bcm430x_ilt_finefreqa[BCM430x_ILT_FINEFREQA_SIZE];
#define BCM430x_ILT_FINEFREQG_SIZE	256
extern const u16 bcm430x_ilt_finefreqg[BCM430x_ILT_FINEFREQG_SIZE];
#define BCM430x_ILT_NOISEA2_SIZE	8
extern const u16 bcm430x_ilt_noisea2[BCM430x_ILT_NOISEA2_SIZE];
#define BCM430x_ILT_NOISEA3_SIZE	8
extern const u16 bcm430x_ilt_noisea3[BCM430x_ILT_NOISEA3_SIZE];
#define BCM430x_ILT_NOISEG1_SIZE	8
extern const u16 bcm430x_ilt_noiseg1[BCM430x_ILT_NOISEG1_SIZE];
#define BCM430x_ILT_NOISEG2_SIZE	8
extern const u16 bcm430x_ilt_noiseg2[BCM430x_ILT_NOISEG2_SIZE];
#define BCM430x_ILT_NOISESCALEG_SIZE	27
extern const u16 bcm430x_ilt_noisescaleg[BCM430x_ILT_NOISESCALEG_SIZE];
#define BCM430x_ILT_SIGMASQR_SIZE	53
extern const u16 bcm430x_ilt_sigmasqr[BCM430x_ILT_SIGMASQR_SIZE];


void bcm430x_ilt_write16(struct bcm430x_private *bcm, u16 offset, u16 val);
u16 bcm430x_ilt_read16(struct bcm430x_private *bcm, u16 offset);
void bcm430x_ilt_write32(struct bcm430x_private *bcm, u16 offset, u32 val);
u32 bcm430x_ilt_read32(struct bcm430x_private *bcm, u16 offset);

#endif /* BCM430x_ILT_H_ */
