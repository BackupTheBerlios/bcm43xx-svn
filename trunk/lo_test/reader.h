#ifndef __READER_H
#define __READER_H
u16 bcm430x_read16(struct bcm430x_private *bcm, u16 offset);
void bcm430x_write16(struct bcm430x_private *bcm, u16 offset, u16 value);
u32 bcm430x_read32(struct bcm430x_private *bcm, u16 offset);
void bcm430x_write32(struct bcm430x_private *bcm, u16 offset, u32 value);
#endif
