#include "bcm430x_initvals.h"

/* bcm430x_initvals_core24_aphy
 * values from: http://bcm-specs.sipsolutions.net/APHYInitVal2
 * revision: 2005-07-13 07:45:07
 */
const struct bcm430x_initval bcm430x_initvals_core24_aphy[] = {
{ .offset = 0x160, .size = 4, .value = 0x03010005, }, /* Set SHM control word to 0x0301 and address to 0x0005 */
{ .offset = 0x164, .size = 4, .value = 0x00020000, }, /* Write to SHM */
{ .offset = 0x124, .size = 4, .value = 0x00000004, },
{ .offset = 0x128, .size = 4, .value = 0x00000000, },
{ .offset = 0x12C, .size = 4, .value = 0x00000000, },
{ .offset = 0x130, .size = 4, .value = 0x00000000, }, /* Set TemplateRam offset to 0x00000000 */
{ .offset = 0x134, .size = 4, .value = 0x000001CB, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00D40000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF000005, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF02FF01, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000010, }, /* Set TemplateRam offset to 0x00000010 */
{ .offset = 0x134, .size = 4, .value = 0x0002028B, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00480000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF000005, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF02FF01, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x10000302, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000CCBB, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000030, }, /* Set TemplateRam offset to 0x00000030 */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000050, }, /* Set TemplateRam offset to 0x00000050 */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000268, }, /* Set TemplateRam offset to 0x00000268 */
{ .offset = 0x134, .size = 4, .value = 0x000208AC, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00500000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xAB000028, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xDABADABA, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000010, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x000A0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000001, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x04014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x968B8482, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x06010103, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000002, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000068, }, /* Set TemplateRam offset to 0x00000068 */
{ .offset = 0x134, .size = 4, .value = 0x000008CB, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00800000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFF0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFFFFFF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000AFD0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000002, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x08014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xA498921C, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xECE0C8B0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00010206, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000468, }, /* Set TemplateRam offset to 0x00000468 */
{ .offset = 0x134, .size = 4, .value = 0x000008CB, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00800000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFF0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFFFFFF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000AFD0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000002, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x08014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xA498921C, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xECE0C8B0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00010206, }, /* Write to TemplateRam */
{ .offset = 0x100, .size = 4, .value = 0x01000000, },
{ .offset = 0x104, .size = 4, .value = 0x01000000, },
{ .offset = 0x108, .size = 4, .value = 0x01000000, },
{ .offset = 0x10C, .size = 4, .value = 0x01000000, },
{ .offset = 0x3E6, .size = 2, .value = 0x0000, },
{ .offset = 0x4A8, .size = 2, .value = 0x0000, },
{ .offset = 0x4AA, .size = 2, .value = 0x0000, },
{ .offset = 0x4A4, .size = 2, .value = 0x12CF, },
{ .offset = 0x4AC, .size = 2, .value = 0x0000, },
{ .offset = 0x4AE, .size = 2, .value = 0xFFFF, },
{ .offset = 0x406, .size = 2, .value = 0x0001, },
{ .offset = 0x406, .size = 2, .value = 0x0000, },
{ .offset = 0x40C, .size = 2, .value = 0x0014, },
{ .offset = 0x406, .size = 2, .value = 0x0101, },
{ .offset = 0x406, .size = 2, .value = 0x0100, },
{ .offset = 0x40C, .size = 2, .value = 0x0014, },
{ .offset = 0x406, .size = 2, .value = 0x0201, },
{ .offset = 0x406, .size = 2, .value = 0x0200, },
{ .offset = 0x40C, .size = 2, .value = 0x0014, },
{ .offset = 0x406, .size = 2, .value = 0x0301, },
{ .offset = 0x406, .size = 2, .value = 0x0300, },
{ .offset = 0x40C, .size = 2, .value = 0x0010, },
{ .offset = 0x406, .size = 2, .value = 0x0000, },
{ .offset = 0x402, .size = 2, .value = 0x0524, },
{ .offset = 0x580, .size = 2, .value = 0xFFFF, },
{ .offset = 0x582, .size = 2, .value = 0xFFFF, },
{ .offset = 0x584, .size = 2, .value = 0xFFFF, },
{ .offset = 0x586, .size = 2, .value = 0xFFFF, },
{ .offset = 0x588, .size = 2, .value = 0xFFFF, },
{ .offset = 0x540, .size = 2, .value = 0x8000, },
{ .offset = 0x520, .size = 2, .value = 0x220C, },
{ .offset = 0x540, .size = 2, .value = 0x8000, },
{ .offset = 0x540, .size = 2, .value = 0x8100, },
{ .offset = 0x520, .size = 2, .value = 0x3A24, },
{ .offset = 0x540, .size = 2, .value = 0x8100, },
{ .offset = 0x540, .size = 2, .value = 0x8200, },
{ .offset = 0x520, .size = 2, .value = 0x523C, },
{ .offset = 0x540, .size = 2, .value = 0x8200, },
{ .offset = 0x540, .size = 2, .value = 0x8300, },
{ .offset = 0x520, .size = 2, .value = 0x5E54, },
{ .offset = 0x540, .size = 2, .value = 0x8300, },
{ .offset = 0x540, .size = 2, .value = 0x8400, },
{ .offset = 0x520, .size = 2, .value = 0x5E5E, },
{ .offset = 0x540, .size = 2, .value = 0x8400, },
{ .offset = 0x540, .size = 2, .value = 0x8500, },
{ .offset = 0x520, .size = 2, .value = 0x5E5E, },
{ .offset = 0x540, .size = 2, .value = 0x8500, },
{ .offset = 0x540, .size = 2, .value = 0x8600, },
{ .offset = 0x520, .size = 2, .value = 0x5E5E, },
{ .offset = 0x540, .size = 2, .value = 0x8600, },
{ .offset = 0x540, .size = 2, .value = 0x8700, },
{ .offset = 0x520, .size = 2, .value = 0x5E5E, },
{ .offset = 0x540, .size = 2, .value = 0x8700, },
{ .offset = 0x612, .size = 2, .value = 0x0001, },
{ .offset = 0x62E, .size = 2, .value = 0xCCCD, },
{ .offset = 0x630, .size = 2, .value = 0x000C, },
{ .offset = 0x600, .size = 2, .value = 0x8004, },
{ .offset = 0x686, .size = 2, .value = 0x02C0, },
{ .offset = 0x680, .size = 2, .value = 0x5070, },
{ .offset = 0x682, .size = 2, .value = 0x0250, },
{ .offset = 0x700, .size = 2, .value = 0x004E, },
{ .offset = 0x688, .size = 2, .value = 0x000B, },
{ .offset = 0x160, .size = 4, .value = 0x03010003, }, /* Set SHM control word to 0x0301 and address to 0x0003 */
{ .offset = 0x164, .size = 4, .value = 0x00100014, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000009, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000080, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00480048, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00640038, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000930, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0301000D, }, /* Set SHM control word to 0x0301 and address to 0x000D */
{ .offset = 0x164, .size = 4, .value = 0x00020002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00040004, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000001D, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010011, }, /* Set SHM control word to 0x0301 and address to 0x0011 */
{ .offset = 0x164, .size = 4, .value = 0x00640064, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0047000E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00640C00, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010015, }, /* Set SHM control word to 0x0301 and address to 0x0015 */
{ .offset = 0x164, .size = 4, .value = 0x05CC0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFFFFFFFF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000000A, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0301001D, }, /* Set SHM control word to 0x0301 and address to 0x001D */
{ .offset = 0x164, .size = 4, .value = 0x00002710, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030100A0, }, /* Set SHM control word to 0x0301 and address to 0x00A0 */
{ .offset = 0x164, .size = 4, .value = 0x01CB0020, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08AB003C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00140084, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000201CF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0034FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000208AF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00640000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01CA0010, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08AA0030, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00080054, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000001CE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x002CFF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000008AE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00440000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01C90008, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08A9002C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0004003C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000001CD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0028FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000008AD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00340000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01C80004, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08A80028, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000030, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000201CC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0028FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000208AC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00300000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x040A00C0, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBADC0070, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x040A013A, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xEE910228, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x006002F2, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00380414, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0102BADC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01140414, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01DEAEEB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x04370022, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBADC0015, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x043700DF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFCE00065, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0011012E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000B846E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00D4BADC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0033846E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00FC5B5E, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030100E0, }, /* Set SHM control word to 0x0301 and address to 0x00E0 */
{ .offset = 0x164, .size = 4, .value = 0x4D435242, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x5345545F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x53535F54, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00004449, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010100, }, /* Set SHM control word to 0x0301 and address to 0x0100 */
{ .offset = 0x164, .size = 4, .value = 0x27100006, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010106, }, /* Set SHM control word to 0x0301 and address to 0x0106 */
{ .offset = 0x164, .size = 4, .value = 0x0C181818, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010120, }, /* Set SHM control word to 0x0301 and address to 0x0120 */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01640176, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400152, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x016D017F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0149015B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01640176, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400152, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x016D017F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0149015B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880191, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x019A0188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x018801A3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880191, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x019A0188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x018801A3, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0301016E, }, /* Set SHM control word to 0x0301 and address to 0x016E */
{ .offset = 0x164, .size = 4, .value = 0xF884C6A5, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF68DEE99, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xD6BDFF0D, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x9154DEB1, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x02036050, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x567DCEA9, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB562E719, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xEC9A4DE6, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1F9D8F45, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFA878940, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB2EBEF15, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFB0B8EC9, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB36741EC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x45EA5FFD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x53F723BF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x9B5BE496, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xE11C75C2, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x4C6A3DAE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x7E416C5A, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x834FF502, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x51F4685C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF908D134, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xAB73E293, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x2A3F6253, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x9552080C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x9D5E4665, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x37A13028, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x2FB50A0F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x24360E09, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xDF3D1B9B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x4E69CD26, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xEA9F7FCD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1D9E121B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x342E5874, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xDCB2362D, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x5BFBB4EE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x764DA4F6, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x7DCEB761, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xDD3E527B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x13975E71, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB968A6F5, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC12C0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xE31F4060, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB6ED79C8, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x8D46D4BE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x724B67D9, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x98D494DE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x854AB0E8, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC52ABB6B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xED164FE5, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x9AD786C5, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x11946655, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xE9108ACF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFE810406, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x7844A0F0, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x4BE325BA, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x5DFEA2F3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x058A80C0, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x21BC3FAD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF1047048, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x77C163DF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x4263AF75, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xE51A2030, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBF6DFD0E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1814814C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC32F2635, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x35A2BEE1, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x2E3988CC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x55F29357, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x7A47FC82, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBAE7C8AC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xE695322B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1998C0A0, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xA37F9ED1, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x547E4466, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0B833BAB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC7298CCA, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x283C6BD3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBCE2A779, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xAD76161D, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x6456DB3B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x141E744E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0C0A92DB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB8E4486C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBD6E9F5D, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC4A643EF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x31A439A8, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF28BD337, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x8B43D532, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xDAB76E59, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB164018C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x49E09CD2, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xACFAD8B4, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xCF25F307, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF48ECAAF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x101847E9, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF0886FD5, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x5C724A6F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x57F13824, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x975173C7, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xA17CCB23, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x3E21E89C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x61DC96DD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0F850D86, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x7C42E090, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xCCAA71C4, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x060590D8, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1C12F701, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x6A5FC2A3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x69D0AEF9, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x99581791, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x27B93A27, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xEB13D938, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x22332BB3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xA970D2BB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x33A70789, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x3C222DB6, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC9201592, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xAAFF8749, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xA57A5078, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x59F8038F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1A170980, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xD73165DA, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xD0B884C6, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x29B082C3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1E115A77, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xA8FC7BCB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x2C3A6DD6, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020003, }, /* Set SHM control word to 0x0002 and address to 0x0003 */
{ .offset = 0x164, .size = 4, .value = 0x0000000F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020004, }, /* Set SHM control word to 0x0002 and address to 0x0004 */
{ .offset = 0x164, .size = 4, .value = 0x000003FF, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020005, }, /* Set SHM control word to 0x0002 and address to 0x0005 */
{ .offset = 0x164, .size = 4, .value = 0x0000000F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020006, }, /* Set SHM control word to 0x0002 and address to 0x0006 */
{ .offset = 0x164, .size = 4, .value = 0x00000007, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020007, }, /* Set SHM control word to 0x0002 and address to 0x0007 */
{ .offset = 0x164, .size = 4, .value = 0x00000004, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020008, }, /* Set SHM control word to 0x0002 and address to 0x0008 */
{ .offset = 0x164, .size = 4, .value = 0x0000FFFF, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020009, }, /* Set SHM control word to 0x0002 and address to 0x0009 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000A, }, /* Set SHM control word to 0x0002 and address to 0x000A */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000B, }, /* Set SHM control word to 0x0002 and address to 0x000B */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000C, }, /* Set SHM control word to 0x0002 and address to 0x000C */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000D, }, /* Set SHM control word to 0x0002 and address to 0x000D */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000E, }, /* Set SHM control word to 0x0002 and address to 0x000E */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000F, }, /* Set SHM control word to 0x0002 and address to 0x000F */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020010, }, /* Set SHM control word to 0x0002 and address to 0x0010 */
{ .offset = 0x164, .size = 4, .value = 0x0000000F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020011, }, /* Set SHM control word to 0x0002 and address to 0x0011 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020012, }, /* Set SHM control word to 0x0002 and address to 0x0012 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020013, }, /* Set SHM control word to 0x0002 and address to 0x0013 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020014, }, /* Set SHM control word to 0x0002 and address to 0x0014 */
{ .offset = 0x164, .size = 4, .value = 0x00000100, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020019, }, /* Set SHM control word to 0x0002 and address to 0x0019 */
{ .offset = 0x164, .size = 4, .value = 0x000003E6, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002001A, }, /* Set SHM control word to 0x0002 and address to 0x001A */
{ .offset = 0x164, .size = 4, .value = 0x000003E6, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002001B, }, /* Set SHM control word to 0x0002 and address to 0x001B */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x0000, .size = 0x00, .value = 0x00000000, }, /* EOL */
};



/* bcm430x_initvals_core24_aphy
 * values from: http://bcm-specs.sipsolutions.net/BGPHYInitVal2
 * revision: 2005-07-13 07:46:07
 */
const struct bcm430x_initval bcm430x_initvals_core24_bgphy[] = {
{ .offset = 0x160, .size = 4, .value = 0x03010005, }, /* Set SHM control word to 0x0301 and address to 0x0005 */
{ .offset = 0x164, .size = 4, .value = 0x00020000, }, /* Write to SHM */
{ .offset = 0x124, .size = 4, .value = 0x00000004, },
{ .offset = 0x128, .size = 4, .value = 0x00000000, },
{ .offset = 0x12C, .size = 4, .value = 0x00000000, },
{ .offset = 0x130, .size = 4, .value = 0x00000000, }, /* Set TemplateRam offset to 0x00000000 */
{ .offset = 0x134, .size = 4, .value = 0x0070040A, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00D4BEEF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF000005, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF02FF01, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000010, }, /* Set TemplateRam offset to 0x00000010 */
{ .offset = 0x134, .size = 4, .value = 0x00E0040A, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0048BEEF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF000005, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF02FF01, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x10000302, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000CCBB, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000030, }, /* Set TemplateRam offset to 0x00000030 */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000050, }, /* Set TemplateRam offset to 0x00000050 */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000268, }, /* Set TemplateRam offset to 0x00000268 */
{ .offset = 0x134, .size = 4, .value = 0x0033846E, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0050BADC, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xAB0000D4, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xDABADABA, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000010, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x000A0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000001, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x04014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x968B8482, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x06010103, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000002, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000068, }, /* Set TemplateRam offset to 0x00000068 */
{ .offset = 0x134, .size = 4, .value = 0x0228040A, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0080BADC, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFF0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFFFFFF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000AFD0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000002, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x04014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x968B8482, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x06010103, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000102, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000468, }, /* Set TemplateRam offset to 0x00000468 */
{ .offset = 0x134, .size = 4, .value = 0x0228040A, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0080BADC, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFF0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFFFFFF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000AFD0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000002, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x04014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x968B8482, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x06010103, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000102, }, /* Write to TemplateRam */
{ .offset = 0x100, .size = 4, .value = 0x01000000, },
{ .offset = 0x104, .size = 4, .value = 0x01000000, },
{ .offset = 0x108, .size = 4, .value = 0x01000000, },
{ .offset = 0x10C, .size = 4, .value = 0x01000000, },
{ .offset = 0x3E6, .size = 2, .value = 0x0022, },
{ .offset = 0x4A8, .size = 2, .value = 0x0000, },
{ .offset = 0x4AA, .size = 2, .value = 0x0000, },
{ .offset = 0x4A4, .size = 2, .value = 0x12CF, },
{ .offset = 0x4AC, .size = 2, .value = 0x0000, },
{ .offset = 0x4AE, .size = 2, .value = 0xFFFF, },
{ .offset = 0x406, .size = 2, .value = 0x0001, },
{ .offset = 0x406, .size = 2, .value = 0x0000, },
{ .offset = 0x40C, .size = 2, .value = 0x0014, },
{ .offset = 0x406, .size = 2, .value = 0x0101, },
{ .offset = 0x406, .size = 2, .value = 0x0100, },
{ .offset = 0x40C, .size = 2, .value = 0x0014, },
{ .offset = 0x406, .size = 2, .value = 0x0201, },
{ .offset = 0x406, .size = 2, .value = 0x0200, },
{ .offset = 0x40C, .size = 2, .value = 0x0014, },
{ .offset = 0x406, .size = 2, .value = 0x0301, },
{ .offset = 0x406, .size = 2, .value = 0x0300, },
{ .offset = 0x40C, .size = 2, .value = 0x0010, },
{ .offset = 0x406, .size = 2, .value = 0x0000, },
{ .offset = 0x402, .size = 2, .value = 0x0524, },
{ .offset = 0x580, .size = 2, .value = 0xFFFF, },
{ .offset = 0x582, .size = 2, .value = 0xFFFF, },
{ .offset = 0x584, .size = 2, .value = 0xFFFF, },
{ .offset = 0x586, .size = 2, .value = 0xFFFF, },
{ .offset = 0x588, .size = 2, .value = 0xFFFF, },
{ .offset = 0x540, .size = 2, .value = 0x8000, },
{ .offset = 0x520, .size = 2, .value = 0x220C, },
{ .offset = 0x540, .size = 2, .value = 0x8000, },
{ .offset = 0x540, .size = 2, .value = 0x8100, },
{ .offset = 0x520, .size = 2, .value = 0x3A24, },
{ .offset = 0x540, .size = 2, .value = 0x8100, },
{ .offset = 0x540, .size = 2, .value = 0x8200, },
{ .offset = 0x520, .size = 2, .value = 0x523C, },
{ .offset = 0x540, .size = 2, .value = 0x8200, },
{ .offset = 0x540, .size = 2, .value = 0x8300, },
{ .offset = 0x520, .size = 2, .value = 0x5E54, },
{ .offset = 0x540, .size = 2, .value = 0x8300, },
{ .offset = 0x540, .size = 2, .value = 0x8400, },
{ .offset = 0x520, .size = 2, .value = 0x5E5E, },
{ .offset = 0x540, .size = 2, .value = 0x8400, },
{ .offset = 0x540, .size = 2, .value = 0x8500, },
{ .offset = 0x520, .size = 2, .value = 0x5E5E, },
{ .offset = 0x540, .size = 2, .value = 0x8500, },
{ .offset = 0x540, .size = 2, .value = 0x8600, },
{ .offset = 0x520, .size = 2, .value = 0x5E5E, },
{ .offset = 0x540, .size = 2, .value = 0x8600, },
{ .offset = 0x540, .size = 2, .value = 0x8700, },
{ .offset = 0x520, .size = 2, .value = 0x5E5E, },
{ .offset = 0x540, .size = 2, .value = 0x8700, },
{ .offset = 0x612, .size = 2, .value = 0x0001, },
{ .offset = 0x62E, .size = 2, .value = 0xA2E9, },
{ .offset = 0x630, .size = 2, .value = 0x000B, },
{ .offset = 0x600, .size = 2, .value = 0x8004, },
{ .offset = 0x686, .size = 2, .value = 0x0B4E, },
{ .offset = 0x680, .size = 2, .value = 0x3E3E, },
{ .offset = 0x682, .size = 2, .value = 0x023E, },
{ .offset = 0x700, .size = 2, .value = 0x003C, },
{ .offset = 0x688, .size = 2, .value = 0x000B, },
{ .offset = 0x160, .size = 4, .value = 0x03010003, }, /* Set SHM control word to 0x0301 and address to 0x0003 */
{ .offset = 0x164, .size = 4, .value = 0x000A00C0, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000014, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000080, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00470047, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00640183, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000930, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0301000D, }, /* Set SHM control word to 0x0301 and address to 0x000D */
{ .offset = 0x164, .size = 4, .value = 0x00020002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00040001, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000001E, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010011, }, /* Set SHM control word to 0x0301 and address to 0x0011 */
{ .offset = 0x164, .size = 4, .value = 0x00640064, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0047000E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00640C00, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010015, }, /* Set SHM control word to 0x0301 and address to 0x0015 */
{ .offset = 0x164, .size = 4, .value = 0x05CC0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFFFFFFFF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000000A, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0301001D, }, /* Set SHM control word to 0x0301 and address to 0x001D */
{ .offset = 0x164, .size = 4, .value = 0x00002710, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030100A0, }, /* Set SHM control word to 0x0301 and address to 0x00A0 */
{ .offset = 0x164, .size = 4, .value = 0x01CB0020, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08AB003C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00140084, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000201CF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0034FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000208AF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00640000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01CA0010, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08AA0030, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00080054, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000001CE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x002CFF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000008AE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00440000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01C90008, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08A9002C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0004003C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000001CD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0028FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000008AD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00340000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01C80004, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08A80028, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000030, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000201CC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0028FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000208AC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00300000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x040A00C0, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBADC0070, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x040A013A, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xEE910228, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x006002F2, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00380414, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0102BADC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01140414, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01DEAEEB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x04370022, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBADC0015, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x043700DF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFCE00065, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0011012E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000B846E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00D4BADC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0033846E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00FC5B5E, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030100E0, }, /* Set SHM control word to 0x0301 and address to 0x00E0 */
{ .offset = 0x164, .size = 4, .value = 0x4D435242, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x5345545F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x53535F54, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00004449, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010100, }, /* Set SHM control word to 0x0301 and address to 0x0100 */
{ .offset = 0x164, .size = 4, .value = 0x27100006, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010106, }, /* Set SHM control word to 0x0301 and address to 0x0106 */
{ .offset = 0x164, .size = 4, .value = 0x0C181818, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010120, }, /* Set SHM control word to 0x0301 and address to 0x0120 */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01640176, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400152, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x016D017F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0149015B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01640176, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400152, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x016D017F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0149015B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880191, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x019A0188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x018801A3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880191, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x019A0188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x018801A3, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0301016E, }, /* Set SHM control word to 0x0301 and address to 0x016E */
{ .offset = 0x164, .size = 4, .value = 0xF884C6A5, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF68DEE99, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xD6BDFF0D, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x9154DEB1, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x02036050, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x567DCEA9, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB562E719, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xEC9A4DE6, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1F9D8F45, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFA878940, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB2EBEF15, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFB0B8EC9, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB36741EC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x45EA5FFD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x53F723BF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x9B5BE496, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xE11C75C2, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x4C6A3DAE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x7E416C5A, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x834FF502, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x51F4685C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF908D134, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xAB73E293, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x2A3F6253, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x9552080C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x9D5E4665, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x37A13028, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x2FB50A0F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x24360E09, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xDF3D1B9B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x4E69CD26, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xEA9F7FCD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1D9E121B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x342E5874, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xDCB2362D, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x5BFBB4EE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x764DA4F6, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x7DCEB761, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xDD3E527B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x13975E71, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB968A6F5, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC12C0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xE31F4060, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB6ED79C8, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x8D46D4BE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x724B67D9, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x98D494DE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x854AB0E8, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC52ABB6B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xED164FE5, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x9AD786C5, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x11946655, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xE9108ACF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFE810406, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x7844A0F0, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x4BE325BA, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x5DFEA2F3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x058A80C0, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x21BC3FAD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF1047048, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x77C163DF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x4263AF75, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xE51A2030, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBF6DFD0E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1814814C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC32F2635, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x35A2BEE1, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x2E3988CC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x55F29357, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x7A47FC82, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBAE7C8AC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xE695322B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1998C0A0, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xA37F9ED1, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x547E4466, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0B833BAB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC7298CCA, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x283C6BD3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBCE2A779, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xAD76161D, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x6456DB3B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x141E744E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0C0A92DB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB8E4486C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBD6E9F5D, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC4A643EF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x31A439A8, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF28BD337, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x8B43D532, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xDAB76E59, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xB164018C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x49E09CD2, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xACFAD8B4, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xCF25F307, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF48ECAAF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x101847E9, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xF0886FD5, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x5C724A6F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x57F13824, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x975173C7, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xA17CCB23, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x3E21E89C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x61DC96DD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0F850D86, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x7C42E090, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xCCAA71C4, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x060590D8, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1C12F701, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x6A5FC2A3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x69D0AEF9, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x99581791, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x27B93A27, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xEB13D938, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x22332BB3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xA970D2BB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x33A70789, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x3C222DB6, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xC9201592, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xAAFF8749, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xA57A5078, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x59F8038F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1A170980, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xD73165DA, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xD0B884C6, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x29B082C3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x1E115A77, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xA8FC7BCB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x2C3A6DD6, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020003, }, /* Set SHM control word to 0x0002 and address to 0x0003 */
{ .offset = 0x164, .size = 4, .value = 0x0000001F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020004, }, /* Set SHM control word to 0x0002 and address to 0x0004 */
{ .offset = 0x164, .size = 4, .value = 0x000003FF, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020005, }, /* Set SHM control word to 0x0002 and address to 0x0005 */
{ .offset = 0x164, .size = 4, .value = 0x0000001F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020006, }, /* Set SHM control word to 0x0002 and address to 0x0006 */
{ .offset = 0x164, .size = 4, .value = 0x00000007, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020007, }, /* Set SHM control word to 0x0002 and address to 0x0007 */
{ .offset = 0x164, .size = 4, .value = 0x00000004, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020008, }, /* Set SHM control word to 0x0002 and address to 0x0008 */
{ .offset = 0x164, .size = 4, .value = 0x0000FFFF, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020009, }, /* Set SHM control word to 0x0002 and address to 0x0009 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000A, }, /* Set SHM control word to 0x0002 and address to 0x000A */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000B, }, /* Set SHM control word to 0x0002 and address to 0x000B */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000C, }, /* Set SHM control word to 0x0002 and address to 0x000C */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000D, }, /* Set SHM control word to 0x0002 and address to 0x000D */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000E, }, /* Set SHM control word to 0x0002 and address to 0x000E */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000F, }, /* Set SHM control word to 0x0002 and address to 0x000F */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020010, }, /* Set SHM control word to 0x0002 and address to 0x0010 */
{ .offset = 0x164, .size = 4, .value = 0x0000001F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020011, }, /* Set SHM control word to 0x0002 and address to 0x0011 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020012, }, /* Set SHM control word to 0x0002 and address to 0x0012 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020013, }, /* Set SHM control word to 0x0002 and address to 0x0013 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020014, }, /* Set SHM control word to 0x0002 and address to 0x0014 */
{ .offset = 0x164, .size = 4, .value = 0x00000100, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020019, }, /* Set SHM control word to 0x0002 and address to 0x0019 */
{ .offset = 0x164, .size = 4, .value = 0x000003E6, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002001A, }, /* Set SHM control word to 0x0002 and address to 0x001A */
{ .offset = 0x164, .size = 4, .value = 0x000003E6, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002001B, }, /* Set SHM control word to 0x0002 and address to 0x001B */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x0000, .size = 0x00, .value = 0x00000000, }, /* EOL */
};



/* bcm430x_initvals_core5_aphy
 * values from: http://bcm-specs.sipsolutions.net/APHYInitVal5
 * revision: 2005-07-12 06:23:27
 */
const struct bcm430x_initval bcm430x_initvals_core5_aphy[] = {
{ .offset = 0x160, .size = 4, .value = 0x03010005, }, /* Set SHM control word to 0x0301 and address to 0x0005 */
{ .offset = 0x164, .size = 4, .value = 0x00050000, }, /* Write to SHM */
{ .offset = 0x124, .size = 4, .value = 0x00000004, },
{ .offset = 0x128, .size = 4, .value = 0x00000000, },
{ .offset = 0x12C, .size = 4, .value = 0x00000000, },
{ .offset = 0x130, .size = 4, .value = 0x00000000, }, /* Set TemplateRam offset to 0x00000000 */
{ .offset = 0x134, .size = 4, .value = 0x000001CB, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00D40000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF000005, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF02FF01, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000010, }, /* Set TemplateRam offset to 0x00000010 */
{ .offset = 0x134, .size = 4, .value = 0x0002028B, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00480000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF000005, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF02FF01, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x10000302, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000CCBB, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000030, }, /* Set TemplateRam offset to 0x00000030 */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000050, }, /* Set TemplateRam offset to 0x00000050 */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000268, }, /* Set TemplateRam offset to 0x00000268 */
{ .offset = 0x134, .size = 4, .value = 0x000208AC, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00500000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xAB000028, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xDABADABA, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000010, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x000A0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000001, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x04014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x968B8482, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x06010103, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000002, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000068, }, /* Set TemplateRam offset to 0x00000068 */
{ .offset = 0x134, .size = 4, .value = 0x000008CB, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00800000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFF0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFFFFFF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000AFD0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000002, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x08014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xA498921C, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xECE0C8B0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00010206, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000468, }, /* Set TemplateRam offset to 0x00000468 */
{ .offset = 0x134, .size = 4, .value = 0x000008CB, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00800000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFF0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFFFFFF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000AFD0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000002, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x08014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xA498921C, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xECE0C8B0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00010206, }, /* Write to TemplateRam */
{ .offset = 0x100, .size = 4, .value = 0x01000000, },
{ .offset = 0x3E6, .size = 2, .value = 0x0000, },
{ .offset = 0x490, .size = 2, .value = 0x0000, },
{ .offset = 0x4A0, .size = 2, .value = 0xE3F9, },
{ .offset = 0x4B0, .size = 2, .value = 0xFDAF, },
{ .offset = 0x4A8, .size = 2, .value = 0xFFFF, },
{ .offset = 0x4A8, .size = 2, .value = 0x0000, },
{ .offset = 0x4AA, .size = 2, .value = 0x0000, },
{ .offset = 0x4A4, .size = 2, .value = 0x12CF, },
{ .offset = 0x4AC, .size = 2, .value = 0x0000, },
{ .offset = 0x4BC, .size = 2, .value = 0x0000, },
{ .offset = 0x4A6, .size = 2, .value = 0x00C7, },
{ .offset = 0x4B6, .size = 2, .value = 0xFFFF, },
{ .offset = 0x4AE, .size = 2, .value = 0xFFFF, },
{ .offset = 0x406, .size = 2, .value = 0x0001, },
{ .offset = 0x406, .size = 2, .value = 0x0000, },
{ .offset = 0x40C, .size = 2, .value = 0x0014, },
{ .offset = 0x406, .size = 2, .value = 0x0000, },
{ .offset = 0x402, .size = 2, .value = 0x0524, },
{ .offset = 0x580, .size = 2, .value = 0xFFFF, },
{ .offset = 0x582, .size = 2, .value = 0xFFFF, },
{ .offset = 0x584, .size = 2, .value = 0xFFFF, },
{ .offset = 0x586, .size = 2, .value = 0xFFFF, },
{ .offset = 0x588, .size = 2, .value = 0xFFFF, },
{ .offset = 0x540, .size = 2, .value = 0x8000, },
{ .offset = 0x520, .size = 2, .value = 0x1206, },
{ .offset = 0x540, .size = 2, .value = 0x8000, },
{ .offset = 0x540, .size = 2, .value = 0x8100, },
{ .offset = 0x520, .size = 2, .value = 0x1F13, },
{ .offset = 0x540, .size = 2, .value = 0x8100, },
{ .offset = 0x540, .size = 2, .value = 0x8200, },
{ .offset = 0x520, .size = 2, .value = 0x2620, },
{ .offset = 0x540, .size = 2, .value = 0x8200, },
{ .offset = 0x540, .size = 2, .value = 0x8300, },
{ .offset = 0x520, .size = 2, .value = 0x2727, },
{ .offset = 0x540, .size = 2, .value = 0x8300, },
{ .offset = 0x612, .size = 2, .value = 0x0001, },
{ .offset = 0x62E, .size = 2, .value = 0xCCCD, },
{ .offset = 0x630, .size = 2, .value = 0x000C, },
{ .offset = 0x600, .size = 2, .value = 0x8004, },
{ .offset = 0x688, .size = 2, .value = 0x000B, },
{ .offset = 0x160, .size = 4, .value = 0x03010004, }, /* Set SHM control word to 0x0301 and address to 0x0004 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000080, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00480048, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00640000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000930, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0301000D, }, /* Set SHM control word to 0x0301 and address to 0x000D */
{ .offset = 0x164, .size = 4, .value = 0x00020002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00040004, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000001D, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010011, }, /* Set SHM control word to 0x0301 and address to 0x0011 */
{ .offset = 0x164, .size = 4, .value = 0x00640064, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0047000E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00642800, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010015, }, /* Set SHM control word to 0x0301 and address to 0x0015 */
{ .offset = 0x164, .size = 4, .value = 0x05CC0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFFFFFFFF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000000A, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0301001D, }, /* Set SHM control word to 0x0301 and address to 0x001D */
{ .offset = 0x164, .size = 4, .value = 0x00002710, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030100A0, }, /* Set SHM control word to 0x0301 and address to 0x00A0 */
{ .offset = 0x164, .size = 4, .value = 0x01CB0020, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08AB0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00140084, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000201CF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000208AF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00640000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01CA0010, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08AA0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00080054, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000001CE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000008AE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00440000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01C90008, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08A90000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0004003C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000001CD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000008AD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00340000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01C80004, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08A80000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000030, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000201CC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000208AC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00300000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x040A00C0, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBADC0070, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x040A013A, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xEE910228, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x006002F2, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00380414, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0102BADC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01140414, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01DEAEEB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x04370022, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBADC0015, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x043700DF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFCE00065, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0011012E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000B846E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00D4BADC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0033846E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00FC5B5E, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030100E0, }, /* Set SHM control word to 0x0301 and address to 0x00E0 */
{ .offset = 0x164, .size = 4, .value = 0x4D435242, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x5345545F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x53535F54, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00004449, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010100, }, /* Set SHM control word to 0x0301 and address to 0x0100 */
{ .offset = 0x164, .size = 4, .value = 0x27100006, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010105, }, /* Set SHM control word to 0x0301 and address to 0x0105 */
{ .offset = 0x164, .size = 4, .value = 0x000001F4, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01070D0D, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010120, }, /* Set SHM control word to 0x0301 and address to 0x0120 */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01640176, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400152, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x016D017F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0149015B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01640176, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400152, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x016D017F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0149015B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880191, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x019A0188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x018801A3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880191, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x019A0188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x018801A3, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030102B2, }, /* Set SHM control word to 0x0301 and address to 0x02B2 */
{ .offset = 0x164, .size = 4, .value = 0x001F0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x001F03FF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030102BA, }, /* Set SHM control word to 0x0301 and address to 0x02BA */
{ .offset = 0x164, .size = 4, .value = 0x001F0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x001F03FF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030102C2, }, /* Set SHM control word to 0x0301 and address to 0x02C2 */
{ .offset = 0x164, .size = 4, .value = 0x001F0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x001F03FF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030102CA, }, /* Set SHM control word to 0x0301 and address to 0x02CA */
{ .offset = 0x164, .size = 4, .value = 0x001F0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x001F03FF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020003, }, /* Set SHM control word to 0x0002 and address to 0x0003 */
{ .offset = 0x164, .size = 4, .value = 0x0000000F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020004, }, /* Set SHM control word to 0x0002 and address to 0x0004 */
{ .offset = 0x164, .size = 4, .value = 0x000003FF, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020005, }, /* Set SHM control word to 0x0002 and address to 0x0005 */
{ .offset = 0x164, .size = 4, .value = 0x0000000F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020006, }, /* Set SHM control word to 0x0002 and address to 0x0006 */
{ .offset = 0x164, .size = 4, .value = 0x00000007, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020007, }, /* Set SHM control word to 0x0002 and address to 0x0007 */
{ .offset = 0x164, .size = 4, .value = 0x00000004, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020008, }, /* Set SHM control word to 0x0002 and address to 0x0008 */
{ .offset = 0x164, .size = 4, .value = 0x0000FFFF, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020009, }, /* Set SHM control word to 0x0002 and address to 0x0009 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000A, }, /* Set SHM control word to 0x0002 and address to 0x000A */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000B, }, /* Set SHM control word to 0x0002 and address to 0x000B */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000C, }, /* Set SHM control word to 0x0002 and address to 0x000C */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000D, }, /* Set SHM control word to 0x0002 and address to 0x000D */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000E, }, /* Set SHM control word to 0x0002 and address to 0x000E */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000F, }, /* Set SHM control word to 0x0002 and address to 0x000F */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020010, }, /* Set SHM control word to 0x0002 and address to 0x0010 */
{ .offset = 0x164, .size = 4, .value = 0x0000000F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020011, }, /* Set SHM control word to 0x0002 and address to 0x0011 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020012, }, /* Set SHM control word to 0x0002 and address to 0x0012 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020013, }, /* Set SHM control word to 0x0002 and address to 0x0013 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020014, }, /* Set SHM control word to 0x0002 and address to 0x0014 */
{ .offset = 0x164, .size = 4, .value = 0x00000100, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020019, }, /* Set SHM control word to 0x0002 and address to 0x0019 */
{ .offset = 0x164, .size = 4, .value = 0x000003E6, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002001A, }, /* Set SHM control word to 0x0002 and address to 0x001A */
{ .offset = 0x164, .size = 4, .value = 0x000003E6, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002001B, }, /* Set SHM control word to 0x0002 and address to 0x001B */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x0000, .size = 0x00, .value = 0x00000000, }, /* EOL */
};



/* bcm430x_bsinitvals_core5_aphy_1
 * use if SB CoreFlagsHI & 0x10000 isn't set
 * values from: http://bcm-specs.sipsolutions.net/APHYBSInitVal5
 * revision: 2005-07-12 06:16:10
 */
const struct bcm430x_initval bcm430x_bsinitvals_core5_aphy_1[] = {
{ .offset = 0x686, .size = 2, .value = 0x02BB, },
{ .offset = 0x680, .size = 2, .value = 0x4B6B, },
{ .offset = 0x682, .size = 2, .value = 0x024B, },
{ .offset = 0x700, .size = 2, .value = 0x0049, },
{ .offset = 0x684, .size = 2, .value = 0x0207, },
{ .offset = 0x160, .size = 4, .value = 0x00010003, }, /* Set SHM control word to 0x0001 and address to 0x0003 */
{ .offset = 0x164, .size = 2, .value = 0x0014, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00010003, }, /* Set SHM control word to 0x0001 and address to 0x0003 */
{ .offset = 0x166, .size = 2, .value = 0x0010, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00010004, }, /* Set SHM control word to 0x0001 and address to 0x0004 */
{ .offset = 0x164, .size = 2, .value = 0x0009, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00010007, }, /* Set SHM control word to 0x0001 and address to 0x0007 */
{ .offset = 0x164, .size = 2, .value = 0x0038, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100A2, }, /* Set SHM control word to 0x0001 and address to 0x00A2 */
{ .offset = 0x164, .size = 2, .value = 0x003C, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100A6, }, /* Set SHM control word to 0x0001 and address to 0x00A6 */
{ .offset = 0x166, .size = 2, .value = 0x0034, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100AB, }, /* Set SHM control word to 0x0001 and address to 0x00AB */
{ .offset = 0x164, .size = 2, .value = 0x0030, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100AF, }, /* Set SHM control word to 0x0001 and address to 0x00AF */
{ .offset = 0x166, .size = 2, .value = 0x002C, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100B4, }, /* Set SHM control word to 0x0001 and address to 0x00B4 */
{ .offset = 0x164, .size = 2, .value = 0x002C, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100B8, }, /* Set SHM control word to 0x0001 and address to 0x00B8 */
{ .offset = 0x166, .size = 2, .value = 0x0028, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100BD, }, /* Set SHM control word to 0x0001 and address to 0x00BD */
{ .offset = 0x164, .size = 2, .value = 0x0028, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100C1, }, /* Set SHM control word to 0x0001 and address to 0x00C1 */
{ .offset = 0x166, .size = 2, .value = 0x0028, }, /* Write to SHM */
{ .offset = 0x0000, .size = 0x00, .value = 0x00000000, }, /* EOL */
};



/* bcm430x_bsinitvals_core5_aphy_2
 * use if SB CoreFlagsHI & 0x10000 is set
 * values from: http://bcm-specs.sipsolutions.net/APHYBSInitVal5
 * revision: 2005-07-12 06:16:10
 */
const struct bcm430x_initval bcm430x_bsinitvals_core5_aphy_2[] = {
{ .offset = 0x686, .size = 2, .value = 0x02C0, },
{ .offset = 0x680, .size = 2, .value = 0x5070, },
{ .offset = 0x682, .size = 2, .value = 0x0250, },
{ .offset = 0x700, .size = 2, .value = 0x004E, },
{ .offset = 0x684, .size = 2, .value = 0x0207, },
{ .offset = 0x160, .size = 4, .value = 0x00010003, }, /* Set SHM control word to 0x0001 and address to 0x0003 */
{ .offset = 0x164, .size = 2, .value = 0x0014, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00010003, }, /* Set SHM control word to 0x0001 and address to 0x0003 */
{ .offset = 0x166, .size = 2, .value = 0x0010, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00010004, }, /* Set SHM control word to 0x0001 and address to 0x0004 */
{ .offset = 0x164, .size = 2, .value = 0x0009, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00010007, }, /* Set SHM control word to 0x0001 and address to 0x0007 */
{ .offset = 0x164, .size = 2, .value = 0x0038, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100A2, }, /* Set SHM control word to 0x0001 and address to 0x00A2 */
{ .offset = 0x164, .size = 2, .value = 0x003C, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100A6, }, /* Set SHM control word to 0x0001 and address to 0x00A6 */
{ .offset = 0x166, .size = 2, .value = 0x0034, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100AB, }, /* Set SHM control word to 0x0001 and address to 0x00AB */
{ .offset = 0x164, .size = 2, .value = 0x0030, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100AF, }, /* Set SHM control word to 0x0001 and address to 0x00AF */
{ .offset = 0x166, .size = 2, .value = 0x002C, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100B4, }, /* Set SHM control word to 0x0001 and address to 0x00B4 */
{ .offset = 0x164, .size = 2, .value = 0x002C, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100B8, }, /* Set SHM control word to 0x0001 and address to 0x00B8 */
{ .offset = 0x166, .size = 2, .value = 0x0028, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100BD, }, /* Set SHM control word to 0x0001 and address to 0x00BD */
{ .offset = 0x164, .size = 2, .value = 0x0028, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100C1, }, /* Set SHM control word to 0x0001 and address to 0x00C1 */
{ .offset = 0x166, .size = 2, .value = 0x0028, }, /* Write to SHM */
{ .offset = 0x0000, .size = 0x00, .value = 0x00000000, }, /* EOL */
};



/* bcm430x_initvals_core5_bgphy
 * values from: http://bcm-specs.sipsolutions.net/BGPHYInitVal5
 * revision: 2005-07-13 07:46:39
 */
const struct bcm430x_initval bcm430x_initvals_core5_bgphy[] = {
{ .offset = 0x160, .size = 4, .value = 0x03010005, }, /* Set SHM control word to 0x0301 and address to 0x0005 */
{ .offset = 0x164, .size = 4, .value = 0x00050000, }, /* Write to SHM */
{ .offset = 0x124, .size = 4, .value = 0x00000004, },
{ .offset = 0x128, .size = 4, .value = 0x00000000, },
{ .offset = 0x12C, .size = 4, .value = 0x00000000, },
{ .offset = 0x130, .size = 4, .value = 0x00000000, }, /* Set TemplateRam offset to 0x00000000 */
{ .offset = 0x134, .size = 4, .value = 0x0070040A, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00D4BEEF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF000005, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF02FF01, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000010, }, /* Set TemplateRam offset to 0x00000010 */
{ .offset = 0x134, .size = 4, .value = 0x00E0040A, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0048BEEF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF000005, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFF02FF01, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x10000302, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000CCBB, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000030, }, /* Set TemplateRam offset to 0x00000030 */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000050, }, /* Set TemplateRam offset to 0x00000050 */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000268, }, /* Set TemplateRam offset to 0x00000268 */
{ .offset = 0x134, .size = 4, .value = 0x0033846E, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0050BADC, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xAB0000D4, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xDABADABA, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000010, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x000A0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000001, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x04014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x968B8482, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x06010103, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000002, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000068, }, /* Set TemplateRam offset to 0x00000068 */
{ .offset = 0x134, .size = 4, .value = 0x0228040A, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0080BADC, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFF0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFFFFFF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000AFD0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000002, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x04014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x968B8482, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x06010103, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000102, }, /* Write to TemplateRam */
{ .offset = 0x130, .size = 4, .value = 0x00000468, }, /* Set TemplateRam offset to 0x00000468 */
{ .offset = 0x134, .size = 4, .value = 0x0228040A, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0080BADC, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFF0000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xFFFFFFFF, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF1181000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x1000F3F2, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0xF3F2F118, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0000AFD0, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x01000000, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x0E000002, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x4D435242, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x5345545F, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x53535F54, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x04014449, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x968B8482, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x06010103, }, /* Write to TemplateRam */
{ .offset = 0x134, .size = 4, .value = 0x00000102, }, /* Write to TemplateRam */
{ .offset = 0x100, .size = 4, .value = 0x01000000, },
{ .offset = 0x3E6, .size = 2, .value = 0x0000, },
{ .offset = 0x490, .size = 2, .value = 0x0000, },
{ .offset = 0x4A0, .size = 2, .value = 0xE3F9, },
{ .offset = 0x4B0, .size = 2, .value = 0xFDAF, },
{ .offset = 0x4A8, .size = 2, .value = 0xFFFF, },
{ .offset = 0x4A8, .size = 2, .value = 0x0000, },
{ .offset = 0x4AA, .size = 2, .value = 0x0000, },
{ .offset = 0x4A4, .size = 2, .value = 0x12CF, },
{ .offset = 0x4AC, .size = 2, .value = 0x0000, },
{ .offset = 0x4BC, .size = 2, .value = 0x0000, },
{ .offset = 0x4A6, .size = 2, .value = 0x00C7, },
{ .offset = 0x4B6, .size = 2, .value = 0xFFFF, },
{ .offset = 0x4AE, .size = 2, .value = 0xFFFF, },
{ .offset = 0x406, .size = 2, .value = 0x0001, },
{ .offset = 0x406, .size = 2, .value = 0x0000, },
{ .offset = 0x40C, .size = 2, .value = 0x0014, },
{ .offset = 0x406, .size = 2, .value = 0x0000, },
{ .offset = 0x402, .size = 2, .value = 0x0524, },
{ .offset = 0x580, .size = 2, .value = 0xFFFF, },
{ .offset = 0x582, .size = 2, .value = 0xFFFF, },
{ .offset = 0x584, .size = 2, .value = 0xFFFF, },
{ .offset = 0x586, .size = 2, .value = 0xFFFF, },
{ .offset = 0x588, .size = 2, .value = 0xFFFF, },
{ .offset = 0x540, .size = 2, .value = 0x8000, },
{ .offset = 0x520, .size = 2, .value = 0x1206, },
{ .offset = 0x540, .size = 2, .value = 0x8000, },
{ .offset = 0x540, .size = 2, .value = 0x8100, },
{ .offset = 0x520, .size = 2, .value = 0x1F13, },
{ .offset = 0x540, .size = 2, .value = 0x8100, },
{ .offset = 0x540, .size = 2, .value = 0x8200, },
{ .offset = 0x520, .size = 2, .value = 0x2620, },
{ .offset = 0x540, .size = 2, .value = 0x8200, },
{ .offset = 0x540, .size = 2, .value = 0x8300, },
{ .offset = 0x520, .size = 2, .value = 0x2727, },
{ .offset = 0x540, .size = 2, .value = 0x8300, },
{ .offset = 0x612, .size = 2, .value = 0x0001, },
{ .offset = 0x62E, .size = 2, .value = 0xA2E9, },
{ .offset = 0x630, .size = 2, .value = 0x000B, },
{ .offset = 0x600, .size = 2, .value = 0x8004, },
{ .offset = 0x688, .size = 2, .value = 0x000B, },
{ .offset = 0x160, .size = 4, .value = 0x03010004, }, /* Set SHM control word to 0x0301 and address to 0x0004 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000080, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00470047, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00640000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000930, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0301000D, }, /* Set SHM control word to 0x0301 and address to 0x000D */
{ .offset = 0x164, .size = 4, .value = 0x00020002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00040001, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000001E, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010011, }, /* Set SHM control word to 0x0301 and address to 0x0011 */
{ .offset = 0x164, .size = 4, .value = 0x00640064, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0047000E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00642800, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010015, }, /* Set SHM control word to 0x0301 and address to 0x0015 */
{ .offset = 0x164, .size = 4, .value = 0x05CC0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFFFFFFFF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000000A, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0301001D, }, /* Set SHM control word to 0x0301 and address to 0x001D */
{ .offset = 0x164, .size = 4, .value = 0x00002710, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030100A0, }, /* Set SHM control word to 0x0301 and address to 0x00A0 */
{ .offset = 0x164, .size = 4, .value = 0x01CB0020, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08AB0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00140084, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000201CF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000208AF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00640000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01CA0010, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08AA0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00080054, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000001CE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000008AE, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00440000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01C90008, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08A90000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0004003C, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000001CD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000008AD, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00340000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01C80004, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFF000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x08A80000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000030, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000201CC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0000FF00, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000208AC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00300000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x040A00C0, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBADC0070, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x040A013A, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xEE910228, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x006002F2, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00380414, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0102BADC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01140414, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01DEAEEB, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x04370022, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xBADC0015, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x043700DF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0xFCE00065, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0011012E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x000B846E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00D4BADC, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0033846E, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00FC5B5E, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030100E0, }, /* Set SHM control word to 0x0301 and address to 0x00E0 */
{ .offset = 0x164, .size = 4, .value = 0x4D435242, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x5345545F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x53535F54, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00004449, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010100, }, /* Set SHM control word to 0x0301 and address to 0x0100 */
{ .offset = 0x164, .size = 4, .value = 0x27100006, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010105, }, /* Set SHM control word to 0x0301 and address to 0x0105 */
{ .offset = 0x164, .size = 4, .value = 0x000001F4, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01070D0D, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x03010120, }, /* Set SHM control word to 0x0301 and address to 0x0120 */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01640176, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400152, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x016D017F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0149015B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400140, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01640176, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01400152, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x016D017F, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x0149015B, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880191, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x019A0188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x018801A3, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880191, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x019A0188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x01880188, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x018801A3, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030102B2, }, /* Set SHM control word to 0x0301 and address to 0x02B2 */
{ .offset = 0x164, .size = 4, .value = 0x001F0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x001F03FF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000002, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030102BA, }, /* Set SHM control word to 0x0301 and address to 0x02BA */
{ .offset = 0x164, .size = 4, .value = 0x001F0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x001F03FF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030102C2, }, /* Set SHM control word to 0x0301 and address to 0x02C2 */
{ .offset = 0x164, .size = 4, .value = 0x001F0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x001F03FF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x030102CA, }, /* Set SHM control word to 0x0301 and address to 0x02CA */
{ .offset = 0x164, .size = 4, .value = 0x001F0000, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x001F03FF, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x164, .size = 4, .value = 0x00000001, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020003, }, /* Set SHM control word to 0x0002 and address to 0x0003 */
{ .offset = 0x164, .size = 4, .value = 0x0000001F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020004, }, /* Set SHM control word to 0x0002 and address to 0x0004 */
{ .offset = 0x164, .size = 4, .value = 0x000003FF, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020005, }, /* Set SHM control word to 0x0002 and address to 0x0005 */
{ .offset = 0x164, .size = 4, .value = 0x0000001F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020006, }, /* Set SHM control word to 0x0002 and address to 0x0006 */
{ .offset = 0x164, .size = 4, .value = 0x00000007, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020007, }, /* Set SHM control word to 0x0002 and address to 0x0007 */
{ .offset = 0x164, .size = 4, .value = 0x00000004, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020008, }, /* Set SHM control word to 0x0002 and address to 0x0008 */
{ .offset = 0x164, .size = 4, .value = 0x0000FFFF, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020009, }, /* Set SHM control word to 0x0002 and address to 0x0009 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000A, }, /* Set SHM control word to 0x0002 and address to 0x000A */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000B, }, /* Set SHM control word to 0x0002 and address to 0x000B */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000C, }, /* Set SHM control word to 0x0002 and address to 0x000C */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000D, }, /* Set SHM control word to 0x0002 and address to 0x000D */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000E, }, /* Set SHM control word to 0x0002 and address to 0x000E */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002000F, }, /* Set SHM control word to 0x0002 and address to 0x000F */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020010, }, /* Set SHM control word to 0x0002 and address to 0x0010 */
{ .offset = 0x164, .size = 4, .value = 0x0000001F, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020011, }, /* Set SHM control word to 0x0002 and address to 0x0011 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020012, }, /* Set SHM control word to 0x0002 and address to 0x0012 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020013, }, /* Set SHM control word to 0x0002 and address to 0x0013 */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020014, }, /* Set SHM control word to 0x0002 and address to 0x0014 */
{ .offset = 0x164, .size = 4, .value = 0x00000100, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00020019, }, /* Set SHM control word to 0x0002 and address to 0x0019 */
{ .offset = 0x164, .size = 4, .value = 0x000003E6, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002001A, }, /* Set SHM control word to 0x0002 and address to 0x001A */
{ .offset = 0x164, .size = 4, .value = 0x000003E6, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x0002001B, }, /* Set SHM control word to 0x0002 and address to 0x001B */
{ .offset = 0x164, .size = 4, .value = 0x00000000, }, /* Write to SHM */
{ .offset = 0x0000, .size = 0x00, .value = 0x00000000, }, /* EOL */
};



/* bcm430x_bsinitvals_core5_bgphy
 * values from: http://bcm-specs.sipsolutions.net/BGPHYBSInitVal5
 * revision: 2005-07-13 18:19:24
 */
const struct bcm430x_initval bcm430x_bsinitvals_core5_bgphy[] = {
{ .offset = 0x686, .size = 2, .value = 0x0B4E, },
{ .offset = 0x680, .size = 2, .value = 0x3E3E, },
{ .offset = 0x682, .size = 2, .value = 0x023E, },
{ .offset = 0x700, .size = 2, .value = 0x003C, },
{ .offset = 0x684, .size = 2, .value = 0x0212, },
{ .offset = 0x160, .size = 4, .value = 0x00010003, }, /* Set SHM control word to 0x0001 and address to 0x0003 */
{ .offset = 0x164, .size = 2, .value = 0x00C0, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00010003, }, /* Set SHM control word to 0x0001 and address to 0x0003 */
{ .offset = 0x166, .size = 2, .value = 0x000A, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00010004, }, /* Set SHM control word to 0x0001 and address to 0x0004 */
{ .offset = 0x164, .size = 2, .value = 0x0014, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x00010007, }, /* Set SHM control word to 0x0001 and address to 0x0007 */
{ .offset = 0x164, .size = 2, .value = 0x0183, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100A2, }, /* Set SHM control word to 0x0001 and address to 0x00A2 */
{ .offset = 0x164, .size = 2, .value = 0x003C, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100A6, }, /* Set SHM control word to 0x0001 and address to 0x00A6 */
{ .offset = 0x166, .size = 2, .value = 0x0034, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100AB, }, /* Set SHM control word to 0x0001 and address to 0x00AB */
{ .offset = 0x164, .size = 2, .value = 0x0030, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100AF, }, /* Set SHM control word to 0x0001 and address to 0x00AF */
{ .offset = 0x166, .size = 2, .value = 0x002C, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100B4, }, /* Set SHM control word to 0x0001 and address to 0x00B4 */
{ .offset = 0x164, .size = 2, .value = 0x002C, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100B8, }, /* Set SHM control word to 0x0001 and address to 0x00B8 */
{ .offset = 0x166, .size = 2, .value = 0x0028, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100BD, }, /* Set SHM control word to 0x0001 and address to 0x00BD */
{ .offset = 0x164, .size = 2, .value = 0x0028, }, /* Write to SHM */
{ .offset = 0x160, .size = 4, .value = 0x000100C1, }, /* Set SHM control word to 0x0001 and address to 0x00C1 */
{ .offset = 0x166, .size = 2, .value = 0x0028, }, /* Write to SHM */
};

/* vim: set ts=8 sw=8 sts=8: */
