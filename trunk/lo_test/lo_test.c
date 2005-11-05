#include "bcm430x.h"
#include "reader.h"


u16 bcm430x_phy_lo_b_r15_loop(struct bcm430x_private *bcm)
{
	int i;
	u16 ret = 0;

	FIXME();
	//FIXME: Why do we loop from 9 to 0 here, instead of from 0 to 9?
	for (i = 9; i > -1; i--){
		bcm430x_phy_write(bcm, 0x0015, 0xAFA0);
		udelay(1);
		bcm430x_phy_write(bcm, 0x0015, 0xEFA0);
		udelay(10);
		bcm430x_phy_write(bcm, 0x0015, 0xFFA0);
		udelay(40);
		ret += bcm430x_phy_read(bcm, 0x002C);
	}

	return ret;
}

void bcm430x_phy_lo_b_measure(struct bcm430x_private *bcm)
{
	const int is_2053_radio = (bcm->current_core->radio->version == 0x2053);
	struct bcm430x_phyinfo *phy = bcm->current_core->phy;
	u16 regstack[12] = { 0 };
	u16 mls;
	u16 fval;
	int i, j;

	regstack[0] = bcm430x_phy_read(bcm, 0x0015);
	regstack[1] = bcm430x_radio_read16(bcm, 0x0052) & 0xFFF0;

	if (is_2053_radio) {
		regstack[2] = bcm430x_phy_read(bcm, 0x000A);
		regstack[3] = bcm430x_phy_read(bcm, 0x002A);
		regstack[4] = bcm430x_phy_read(bcm, 0x0035);
		regstack[5] = bcm430x_phy_read(bcm, 0x0003);
		regstack[6] = bcm430x_phy_read(bcm, 0x0001);
		regstack[7] = bcm430x_phy_read(bcm, 0x0030);

		regstack[8] = bcm430x_radio_read16(bcm, 0x0043);
		regstack[9] = bcm430x_radio_read16(bcm, 0x007A);
		regstack[10] = bcm430x_read16(bcm, 0x03EC);
		regstack[11] = bcm430x_radio_read16(bcm, 0x0052) & 0x00F0;

		bcm430x_phy_write(bcm, 0x0030, 0x00FF);
		bcm430x_write16(bcm, 0x03EC, 0x3F3F);
		bcm430x_phy_write(bcm, 0x0035, regstack[4] & 0xFF7F);
		bcm430x_radio_write16(bcm, 0x007A, regstack[9] & 0xFFF0);
	}
	bcm430x_phy_write(bcm, 0x0015, 0xB000);
	bcm430x_phy_write(bcm, 0x002B, 0x0004);

	if (is_2053_radio) {
		bcm430x_phy_write(bcm, 0x002B, 0x0203);
		bcm430x_phy_write(bcm, 0x002A, 0x08A3);
	}

	phy->minlowsig[0] = 0xFFFF;

	for (i = 0; i < 4; i++) {
		bcm430x_radio_write16(bcm, 0x0052, regstack[1] | i);
		bcm430x_phy_lo_b_r15_loop(bcm);
	}
	for (i = 0; i < 10; i++) {
		bcm430x_radio_write16(bcm, 0x0052, regstack[1] | i);
		mls = bcm430x_phy_lo_b_r15_loop(bcm) / 10;
		if (mls < phy->minlowsig[0]) {
			phy->minlowsig[0] = mls;
			phy->minlowsigpos[0] = i;
		}
	}
	bcm430x_radio_write16(bcm, 0x0052, regstack[1] | phy->minlowsigpos[0]);

	phy->minlowsig[1] = 0xFFFF;

	for (i = -4; i < 5; i += 2) {
		for (j = -4; j < 5; j += 2) {
			if (j < 0)
				fval = (0x0100 * i) + j + 0x0100;
			else
				fval = (0x0100 * i) + j;
			bcm430x_phy_write(bcm, 0x002F, fval);
			mls = bcm430x_phy_lo_b_r15_loop(bcm) / 10;
			if (mls < phy->minlowsig[1]) {
				phy->minlowsig[1] = mls;
				phy->minlowsigpos[1] = fval;
			}
		}
	}
	phy->minlowsigpos[1] += 0x0101;

	bcm430x_phy_write(bcm, 0x002F, phy->minlowsigpos[1]);
	if (is_2053_radio) {
		bcm430x_phy_write(bcm, 0x000A, regstack[2]);
		bcm430x_phy_write(bcm, 0x002A, regstack[3]);
		bcm430x_phy_write(bcm, 0x0035, regstack[4]);
		bcm430x_phy_write(bcm, 0x0003, regstack[5]);
		bcm430x_phy_write(bcm, 0x0001, regstack[6]);
		bcm430x_phy_write(bcm, 0x0030, regstack[7]);

		bcm430x_radio_write16(bcm, 0x0043, regstack[8]);
		bcm430x_radio_write16(bcm, 0x007A, regstack[9]);

		bcm430x_radio_write16(bcm, 0x0052,
		                      (bcm430x_radio_read16(bcm, 0x0052) & 0x000F)
				      | regstack[11]);

		bcm430x_write16(bcm, 0x03EC, regstack[10]);
	}
	bcm430x_phy_write(bcm, 0x0015, regstack[0]);
}


u16 bcm430x_phy_lo_g_deviation_subval(struct bcm430x_private *bcm, u16 control)
{
	if (bcm->current_core->phy->connected) {
		bcm430x_phy_write(bcm, 0x15, 0xE300);
		control <<= 8;
		bcm430x_phy_write(bcm, 0x0812, control | 0x00B0);
		udelay(5);
		bcm430x_phy_write(bcm, 0x0812, control | 0x00B2);
		udelay(2);
		bcm430x_phy_write(bcm, 0x0812, control | 0x00B4);
		udelay(4);
		bcm430x_phy_write(bcm, 0x0015, 0xF300);
		udelay(8);
	} else {
		bcm430x_phy_write(bcm, 0x0015, control | 0xEFA0);
		udelay(2);
		bcm430x_phy_write(bcm, 0x0015, control | 0xEFE0);
		udelay(4);
		bcm430x_phy_write(bcm, 0x0015, control | 0xFFE0);
		udelay(8);
	}

	return bcm430x_phy_read(bcm, 0x002D);
}

 u32 bcm430x_phy_lo_g_singledeviation(struct bcm430x_private *bcm, u16 control)
{
	int i;
	u32 ret = 0;

	for (i = 0; i < 8; i++)
		ret += bcm430x_phy_lo_g_deviation_subval(bcm, control);

	return ret;
}

/* Write the LocalOscillator CONTROL */

void bcm430x_lo_write(struct bcm430x_private *bcm,
		      struct bcm430x_lopair *pair)
{
	u16 value;

	value = (u8)(pair->low);
	value |= ((u8)(pair->high)) << 8;

#ifdef BCM430x_DEBUG
	/* Sanity check. */
	if (pair->low < -8 || pair->low > 8 ||
	    pair->high < -8 || pair->high > 8) {
		printk(KERN_WARNING PFX
		       "WARNING: Writing invalid LOpair "
		       "(low: %d, high: %d, index: %lu)\n",
		       pair->low, pair->high,
		       (unsigned long)(pair - bcm->current_core->phy->_lo_pairs));
		dump_stack();
	}
#endif

	bcm430x_phy_write(bcm, BCM430x_PHY_G_LO_CONTROL, value);
}


struct bcm430x_lopair * bcm430x_find_lopair(struct bcm430x_private *bcm,
					    u16 baseband_attenuation,
					    u16 radio_attenuation,
					    u16 tx)
{
	const u8 dict[10] = { 11, 10, 11, 12, 13, 12, 13, 12, 13, 12 };
	struct bcm430x_phyinfo *phy = bcm->current_core->phy;

	if (baseband_attenuation > 6)
		baseband_attenuation = 6;
	assert(radio_attenuation < 10);
	assert(tx == 0 || tx == 3);

	if (tx == 3) {
		return bcm430x_get_lopair(phy,
					  radio_attenuation,
					  baseband_attenuation);
	}
	return bcm430x_get_lopair(phy, dict[radio_attenuation], baseband_attenuation);
}


struct bcm430x_lopair * bcm430x_current_lopair(struct bcm430x_private *bcm)
{
	return bcm430x_find_lopair(bcm,
				   bcm->current_core->radio->txpower[0],
				   bcm->current_core->radio->txpower[1],
				   bcm->current_core->radio->txpower[2]);
}

/* Adjust B/G LO */
void bcm430x_phy_lo_adjust(struct bcm430x_private *bcm, int fixed)
{
	struct bcm430x_lopair *pair;

	if (fixed) {
		/* Use fixed values. Only for initialization. */
		pair = bcm430x_find_lopair(bcm, 2, 3, 0);
	} else
		pair = bcm430x_current_lopair(bcm);
	bcm430x_lo_write(bcm, pair);
}


void bcm430x_phy_lo_g_measure_txctl2(struct bcm430x_private *bcm)
{
	u16 txctl2 = 0, i;
	u32 smallest, tmp;

	bcm430x_radio_write16(bcm, 0x0052, 0x0000);
	udelay(10);
	smallest = bcm430x_phy_lo_g_singledeviation(bcm, 0);
	for (i = 0; i < 16; i++) {
		bcm430x_radio_write16(bcm, 0x0052, i);
		udelay(10);
		tmp = bcm430x_phy_lo_g_singledeviation(bcm, 0);
		if (tmp < smallest) {
			smallest = tmp;
			txctl2 = i;
		}
	}
	bcm->current_core->radio->txpower[3] = txctl2;
}


void bcm430x_phy_lo_g_state(struct bcm430x_private *bcm,
			    const struct bcm430x_lopair *in_pair,
			    struct bcm430x_lopair *out_pair,
			    u16 r27)
{
	struct bcm430x_lopair transitions[8] = {
		{ .high =  1,  .low =  1, },
		{ .high =  1,  .low =  0, },
		{ .high =  1,  .low = -1, },
		{ .high =  0,  .low = -1, },
		{ .high = -1,  .low = -1, },
		{ .high = -1,  .low =  0, },
		{ .high = -1,  .low =  1, },
		{ .high =  0,  .low =  1, },
	};
	struct bcm430x_lopair transition;
	struct bcm430x_lopair result = {
		.high = in_pair->high,
		.low = in_pair->low,
	};
	int i = 12, j, lowered = 1, state = 0;
	int index;
	u32 deviation, tmp;

	/* Note that in_pair and out_pair can point to the same pair. Be careful. */

#ifdef BCM430x_DEBUG
	{
		/* Revert the poison values. We must begin at 0. */
		if (result.low == -20) {
			assert(result.high == -20);
			result.low = 0;
			result.high = 0;
		}
	}
#endif /* BCM430x_DEBUG */

	bcm430x_lo_write(bcm, &result);
	deviation = bcm430x_phy_lo_g_singledeviation(bcm, r27);
	while ((i--) && (lowered == 1)) {
		lowered = 0;
		assert(state >= 0 && state <= 8);
		if (state == 0) {
			/* Initial state */
			for (j = 0; j < 8; j++) {
				index = j;
				transition.high = result.high + transitions[index].high;
				transition.low = result.low + transitions[index].low;
				if ((abs(transition.low) < 9) && (abs(transition.high) < 9)) {
					bcm430x_lo_write(bcm, &transition);
					tmp = bcm430x_phy_lo_g_singledeviation(bcm, r27);
					if (tmp < deviation) {
						deviation = tmp;
						state = index + 1;
						lowered = 1;

						result.high = transition.high;
						result.low = transition.low;
					}
				}
			}
		} else if (state % 2 == 0) {
			for (j = -1; j < 2; j += 2) {
				index = state + j;
				assert(index >= 1 && index <= 9);
				if (index > 8)
					index = 1;
				index -= 1;
				transition.high = result.high + transitions[index].high;
				transition.low = result.low + transitions[index].low;
				if ((abs(transition.low) < 9) && (abs(transition.high) < 9)) {
					bcm430x_lo_write(bcm, &transition);
					tmp = bcm430x_phy_lo_g_singledeviation(bcm, r27);
					if (tmp < deviation) {
						deviation = tmp;
						state = index + 1;
						lowered = 1;

						result.high = transition.high;
						result.low = transition.low;
					}
				}
			}
		} else {
			for (j = -2; j < 3; j += 4) {
				index = state + j;
				assert(index >= -1 && index <= 9);
				if (index > 8)
					index = 1;
				else if (index < 1)
					index = 7;
				index -= 1;
				transition.high = result.high + transitions[index].high;
				transition.low = result.low + transitions[index].low;
				if ((abs(transition.low) < 9) && (abs(transition.high) < 9)) {
					bcm430x_lo_write(bcm, &transition);
					tmp = bcm430x_phy_lo_g_singledeviation(bcm, r27);
					if (tmp < deviation) {
						deviation = tmp;
						state = index + 1;
						lowered = 1;

						result.high = transition.high;
						result.low = transition.low;
					}
				}
			}	
		}
	}
	out_pair->high = result.high;
	out_pair->low = result.low;
}

