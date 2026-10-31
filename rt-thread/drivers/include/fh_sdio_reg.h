/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12               the first version
 *
 */

#ifndef __FH_SDIO_REG_H__
#define __FH_SDIO_REG_H__

enum
{
    CTRL    = 0x0,   /** Control */
    PWREN   = 0x4,   /** Power-enable */
    CLKDIV  = 0x8,   /** Clock divider */
    CLKSRC  = 0xC,   /** Clock source */
    CLKENA  = 0x10,  /** Clock enable */
    TMOUT   = 0x14,  /** Timeout */
    CTYPE   = 0x18,  /** Card type */
    BLKSIZ  = 0x1C,  /** Block Size */
    BYTCNT  = 0x20,  /** Byte count */
    INTMSK  = 0x24,  /** Interrupt Mask */
    CMDARG  = 0x28,  /** Command Argument */
    CMD     = 0x2C,  /** Command */
    RESP0   = 0x30,  /** Response 0 */
    RESP1   = 0x34,  /** Response 1 */
    RESP2   = 0x38,  /** Response 2 */
    RESP3   = 0x3C,  /** Response 3 */
    MINTSTS = 0x40,  /** Masked interrupt status */
    RINTSTS = 0x44,  /** Raw interrupt status */
    STATUS  = 0x48,  /** Status */
    FIFOTH  = 0x4C,  /** FIFO threshold */
    CDETECT = 0x50,  /** Card detect */
    WRTPRT  = 0x54,  /** Write protect */
    GPIO    = 0x58,  /** General Purpose IO */
    TCBCNT  = 0x5C,  /** Transferred CIU byte count */
    TBBCNT  = 0x60,  /** Transferred host/DMA to/from byte count */
    DEBNCE  = 0x64,  /** Card detect debounce */
    USRID   = 0x68,  /** User ID */
    VERID   = 0x6C,  /** Version ID */
    HCON    = 0x70,  /** Hardware Configuration */
    UHSREG  = 0x74,  /** Reserved */
    BMOD    = 0x80,  /** Bus mode Register */
    PLDMND  = 0x84,  /** Poll Demand */
    DBADDR  = 0x88,  /** Descriptor Base Address */
    IDSTS   = 0x8C,  /** Internal DMAC Status */
    IDINTEN = 0x90,  /** Internal DMAC Interrupt Enable */
    DSCADDR = 0x94,  /** Current Host Descriptor Address */
    BUFADDR = 0x98,  /** Current Host Buffer Address */
    FIFODAT = 0x200, /** FIFO data read write */
};

/* Control register definitions */
#define CTRL_RESET 0x00000001
#define FIFO_RESET 0x00000002
#define DMA_RESET 0x00000004
#define INT_ENABLE 0x00000010
#define READ_WAIT 0x00000040
#define CTRL_USE_IDMAC 0x02000000

/* Command register defines */
#define CMD_START_BIT (1<<31)
#define CMD_HOLD_BIT  (1<<29)
#define CMD_SEND_INITIALIZATION (1<<15)
#define CMD_WAIT_PRVDATA_COMP (1<<13)
#define CMD_CHECK_RESPONSE_CRC (1<<8)
#define CMD_WAIT_RESPONSE_EXPECT (1<<6)

/* Status(0x48) register defines */
#define STATUS_DATA_BUSY_BIT (1<<9)

/* Interrupt mask defines */
#define INTMSK_CDETECT 0x00000001
#define INTMSK_RESP_ERR 0x00000002
#define INTMSK_CMD_DONE 0x00000004
#define INTMSK_DAT_OVER 0x00000008
#define INTMSK_TXDR 0x00000010
#define INTMSK_RXDR 0x00000020
#define INTMSK_RCRC 0x00000040
#define INTMSK_DCRC 0x00000080
#define INTMSK_RTO 0x00000100
#define INTMSK_DTO 0x00000200
#define INTMSK_HTO 0x00000400
#define INTMSK_VSI INTMSK_HTO  /* VSI => Voltage Switch Interrupt */
#define INTMSK_FRUN 0x00000800
#define INTMSK_HLE 0x00001000
#define INTMSK_SBE 0x00002000
#define INTMSK_ACD 0x00004000
#define INTMSK_EBE 0x00008000
#define INTMSK_SDIO 0x00010000
#define INTMSK_DAT_ALL (INTMSK_DAT_OVER | INTMSK_DCRC | INTMSK_SBE | INTMSK_EBE)
#define INTMASK_ERROR                                                        \
    (INTMSK_RESP_ERR | INTMSK_RCRC | INTMSK_DCRC | INTMSK_RTO | INTMSK_DTO | \
     INTMSK_HTO | INTMSK_FRUN | INTMSK_HLE | INTMSK_SBE | INTMSK_EBE)

/*BMOD register define */
#define BMOD_SWR 0x00000001
#define BMOD_DE 0x00000080
#define BMOD_BURST_INCR 0x00000002

/* for STATUS register */
#define GET_FIFO_COUNT(x) (((x)&0x3ffe0000) >> 17)
#define GET_FIFO_DEPTH(x) ((((x)&0x0FFF0000) >> 16) + 1)

/* for IDMA intr register */
#define IDMAINTBITS 0x337

/* Define Card status bits (R1 response) */
#define R1CS_ADDRESS_OUT_OF_RANGE 0x80000000
#define R1CS_ADDRESS_MISALIGN 0x40000000
#define R1CS_BLOCK_LEN_ERR 0x20000000
#define R1CS_ERASE_SEQ_ERR 0x10000000
#define R1CS_ERASE_PARAM 0x08000000
#define R1CS_WP_VIOLATION 0x04000000
#define R1CS_CARD_IS_LOCKED 0x02000000
#define R1CS_LCK_UNLCK_FAILED 0x01000000
#define R1CS_COM_CRC_ERROR 0x00800000
#define R1CS_ILLEGAL_COMMAND 0x00400000
#define R1CS_CARD_ECC_FAILED 0x00200000
#define R1CS_CC_ERROR 0x00100000
#define R1CS_ERROR 0x00080000
#define R1CS_UNDERRUN 0x00040000
#define R1CS_OVERRUN 0x00020000
#define R1CS_CSD_OVERWRITE 0x00010000
#define R1CS_WP_ERASE_SKIP 0x00008000
#define R1CS_RESERVED_0 0x00004000
#define R1CS_ERASE_RESET 0x00002000
#define R1CS_CURRENT_STATE_MASK 0x00001e00
#define R1CS_READY_FOR_DATA 0x00000100
#define R1CS_SWITCH_ERROR 0x00000080
#define R1CS_RESERVED_1 0x00000040
#define R1CS_APP_CMD 0x00000020
#define R1CS_RESERVED_2 0x00000010
#define R1CS_APP_SPECIFIC_MASK 0x0000000c
#define R1CS_MANUFAC_TEST_MASK 0x00000003
#define R1CS_ERROR_OCCURED_MAP 0xfdffa080
#define R1CS_CURRENT_STATE(x) (((x)&R1CS_CURRENT_STATE_MASK) >> 9)

/* R5 response */
#define R5_IO_GEN_ERR 0x00000800
#define R5_IO_FUNC_ERR 0x00000200
#define R5_IO_OUT_RANGE 0x00000100
#define R5_IO_ERR_BITS (R5_IO_GEN_ERR | R5_IO_FUNC_ERR | R5_IO_OUT_RANGE)

/*voltage definition*/
#define MMC_VDD_165_195         0x00000080      /* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21           0x00000100      /* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22           0x00000200      /* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23           0x00000400      /* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24           0x00000800      /* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25           0x00001000      /* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26           0x00002000      /* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27           0x00004000      /* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28           0x00008000      /* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29           0x00010000      /* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30           0x00020000      /* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31           0x00040000      /* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32           0x00080000      /* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33           0x00100000      /* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34           0x00200000      /* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35           0x00400000      /* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36           0x00800000      /* VDD voltage 3.5 ~ 3.6 */

enum
{
    NONE_TYPE = 0,
    SD_TYPE,
    SD_2_0_TYPE,
    SDIO_TYPE,
};

enum
{
    CARD_STATE_EMPTY = -1,
    CARD_STATE_IDLE  = 0,
    CARD_STATE_READY = 1,
    CARD_STATE_IDENT = 2,
    CARD_STATE_STBY  = 3,
    CARD_STATE_TRAN  = 4,
    CARD_STATE_DATA  = 5,
    CARD_STATE_RCV   = 6,
    CARD_STATE_PRG   = 7,
    CARD_STATE_DIS   = 8,
    CARD_STATE_INA   = 9
};

enum DmaDescriptorDES1  /* Buffer's size field of Descriptor */
{
    DescBuf2SizMsk = 0x03FFE000, /* Mask for Buffer2 Size 25:13   */
    DescBuf2SizeShift = 13, /* Shift value for Buffer2 Size */
    DescBuf1SizMsk = 0x00001FFF, /* Mask for Buffer1 Size 12:0    */
    DescBuf1SizeShift = 0, /* Shift value for Buffer2 Size */
};

enum DmaDescriptorDES0  /* Control and status word of DMA descriptor DES0 */
{
    DescOwnByDma = 0x80000000, /* (OWN)Descriptor is owned by DMA engine 31   */
    DescCardErrSummary =
        0x40000000, /* Indicates EBE/RTO/RCRC/SBE/DRTO/DCRC/RE             30 */
    DescEndOfRing =
        0x00000020, /* A "1" indicates End of Ring for Ring Mode           05 */
    DescSecAddrChained =
        0x00000010, /* A "1" indicates DES3 contains Next Desc Address     04 */
    DescFirstDesc = 0x00000008, /* A "1" indicates this Desc contains first 03
                       buffer of the data */
    DescLastDesc =
        0x00000004, /* A "1" indicates buffer pointed to by this this      02
                   Desc contains last buffer of Data */
    DescDisInt =
        0x00000002, /* A "1" in this field disables the RI/TI of IDSTS     01
                   for data that ends in the buffer pointed to by
                   this descriptor */
};



#define ONE_BIT_MODE  (0)
#define FOUR_BIT_MODE (1)

#define synopmob_set_bits(reg, bit_id)   do {*((volatile unsigned int *)(reg)) |= ((unsigned int)(bit_id)); } while (0)
#define synopmob_clear_bits(reg, bit_id) do {*((volatile unsigned int *)(reg)) &= (~((unsigned int)(bit_id))); } while (0)
#define synopmob_set_register(reg, val)  do {*((volatile unsigned int *)(reg)) = (val); } while (0)
#define synopmob_read_register(reg) (*((volatile unsigned int *)(reg)))

#endif
