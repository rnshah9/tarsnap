#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

int
main(void)
{
	uint8x16_t data;
	uint8x16_t key = {0};
	uint8x16_t output;
	uint32x4_t lanes = {0};
	uint8_t arr[16] = {0};

	data = vld1q_u8(arr);
	output = vaeseq_u8(data, key);
	(void)output; /* UNUSED */

	lanes = vdupq_laneq_u32(lanes, 0);

	return (0);
}
