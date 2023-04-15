#include <common.h>
#include <ace_sfr.h>


#if (defined(CONFIG_S5PV210) || defined(CONFIG_S5PC110) || defined(CONFIG_S5PV310) || defined(CONFIG_S5PC210))

/*****************************************************************
	Definitions
*****************************************************************/
#define ACE_read_sfr(_sfr_)	\
	(*(volatile unsigned int*)(ACE_SFR_BASE + _sfr_))
#define ACE_write_sfr(_sfr_, _val_)	\
	do {*(volatile unsigned int*)(ACE_SFR_BASE + _sfr_)	\
		= (unsigned int)(_val_);} while(0)

/* SHA1 value for the message of zero length */
const unsigned char sha1_digest_emptymsg[20] = {
	0xDA, 0x39, 0xA3, 0xEE, 0x5E, 0x6B, 0x4B, 0x0D,
	0x32, 0x55, 0xBF, 0xFF, 0x95, 0x60, 0x18, 0x90,
	0xAF, 0xD8, 0x07, 0x09};


/*****************************************************************
	Functions
*****************************************************************/
/**
 * @brief	This function computes hash value of input (pBuf[0]..pBuf[buflen-1]).
 *
 * @param	pOut	A pointer to the output buffer. When operation is completed
 * 			20 bytes are copied to pOut[0]...pOut[19]. Thus, a user
 * 			should allocate at least 20 bytes at pOut in advance.
 * @param	pBuf	A pointer to the input buffer
 * @param	bufLen	Byte length of input buffer
 *
 * @return	0	Success
 *
 * @remark	This function assumes that pBuf is a physical address of input buffer.
 *
 * @version V1.00
 * @b Revision History
 *	- V01.00	2009.11.13/djpark	Initial Version
 *	- V01.10	2010.10.19/djpark	Modification to support C210/V310
 */
int ace_hash_sha1_digest (
	unsigned char*	pOut,
	unsigned char*	pBuf,
	unsigned int	bufLen
)
{
	unsigned int reg;
	unsigned int* pDigest;

	if (bufLen == 0) {
		/* ACE H/W cannot compute hash value for empty string */
		memcpy(pOut, sha1_digest_emptymsg, 20);
		return 0;
	}

	/* Flush HRDMA */
	ACE_write_sfr(ACE_FC_HRDMAC, ACE_FC_HRDMACFLUSH_ON);
	ACE_write_sfr(ACE_FC_HRDMAC, ACE_FC_HRDMACFLUSH_OFF);

	/* Set byte swap of data in */
	ACE_write_sfr(ACE_HASH_BYTESWAP,
		ACE_HASH_SWAPDI_ON | ACE_HASH_SWAPDO_ON | ACE_HASH_SWAPIV_ON);

	/* Select Hash input mux as external source */
	reg = ACE_read_sfr(ACE_FC_FIFOCTRL);
	reg = (reg & ~ACE_FC_SELHASH_MASK) | ACE_FC_SELHASH_EXOUT;
	ACE_write_sfr(ACE_FC_FIFOCTRL, reg);

	/* Set Hash as SHA1 and start Hash engine */
	reg = ACE_HASH_ENGSEL_SHA1HASH | ACE_HASH_STARTBIT_ON;
	ACE_write_sfr(ACE_HASH_CONTROL, reg);

	/* Enable FIFO mode */
	ACE_write_sfr(ACE_HASH_FIFO_MODE, ACE_HASH_FIFO_ON);

	/* Set message length */
	ACE_write_sfr(ACE_HASH_MSGSIZE_LOW, bufLen);
	ACE_write_sfr(ACE_HASH_MSGSIZE_HIGH, 0);

	/* Set HRDMA */
        ACE_write_sfr(ACE_FC_HRDMAS, (unsigned int)virt_to_phys(pBuf));
	ACE_write_sfr(ACE_FC_HRDMAL, bufLen);

	while ((ACE_read_sfr(ACE_HASH_STATUS) & ACE_HASH_MSGDONE_MASK)
			== ACE_HASH_MSGDONE_OFF);

	/* Clear MSG_DONE bit */
	ACE_write_sfr(ACE_HASH_STATUS, ACE_HASH_MSGDONE_ON);

	/* Read hash result */
	pDigest = (unsigned int*)pOut;
	pDigest[0] = ACE_read_sfr(ACE_HASH_RESULT1);
	pDigest[1] = ACE_read_sfr(ACE_HASH_RESULT2);
	pDigest[2] = ACE_read_sfr(ACE_HASH_RESULT3);
	pDigest[3] = ACE_read_sfr(ACE_HASH_RESULT4);
	pDigest[4] = ACE_read_sfr(ACE_HASH_RESULT5);

	/* Clear HRDMA pending bit */
	ACE_write_sfr(ACE_FC_INTPEND, ACE_FC_HRDMA);

	return 0;
}

#endif

