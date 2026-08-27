/*
 * Minimal TC1791 special-function register definitions used by this project.
 *
 * The original DaVe-generated tc1791.h is not distributed with the project.
 * Register addresses below are limited deliberately to the SFRs referenced by
 * MAIN.c, CAN.c and FLASH.c.  They match the TC179x AUDO MAX register map and
 * the absolute accesses in the original bootloader.bin.
 */

#ifndef TC1791_MINIMAL_H
#define TC1791_MINIMAL_H

typedef union {
	unsigned int reg;
} TC1791_REG32_t;

typedef union {
	unsigned int reg;
	struct {
		unsigned int ENDINIT : 1;
		unsigned int : 31;
	} bits;
} WDT_CON0_t;

typedef union {
	unsigned int reg;
	struct {
		unsigned int : 2;
		unsigned int IR : 1;
		unsigned int DR : 1;
		unsigned int : 28;
	} bits;
} WDT_CON1_t;

typedef union {
	unsigned int reg;
	struct {
		unsigned int VCOBYP : 1;
		unsigned int : 3;
		unsigned int SETFINDIS : 1;
		unsigned int : 27;
	} bits;
} SCU_PLLCON0_t;

typedef union {
	unsigned int reg;
	struct {
		unsigned int VCOBYST : 1;
		unsigned int PWDSTAT : 1;
		unsigned int VCOLOCK : 1;
		unsigned int : 29;
	} bits;
} SCU_PLLSTAT_t;

typedef union {
	unsigned int reg;
	struct {
		unsigned int : 8;
		unsigned int BUSY : 1;
		unsigned int : 23;
	} bits;
} CAN_PANCTR_t;

typedef union {
	unsigned int reg;
	struct {
		unsigned int PBUSY : 1;
		unsigned int D0BUSY : 1;
		unsigned int D1BUSY : 1;
		unsigned int : 29;
	} bits;
} FLASHn_FSR_t;

#define TC1791_REG32(address) (*(volatile TC1791_REG32_t *)(address))

#define STM_TIM0       TC1791_REG32(0xF0000210U)

#define CAN_CLC        TC1791_REG32(0xF0004000U)
#define CAN_FDR        TC1791_REG32(0xF000400CU)
#define CAN_PANCTR     (*(volatile CAN_PANCTR_t *)0xF00041C4U)
#define CAN_NCR0       TC1791_REG32(0xF0004200U)
#define CAN_NIPR0      TC1791_REG32(0xF0004208U)
#define CAN_NPCR0      TC1791_REG32(0xF000420CU)
#define CAN_NBTR0      TC1791_REG32(0xF0004210U)
#define CAN_NECNT0     TC1791_REG32(0xF0004214U)
#define CAN_NFCR0      TC1791_REG32(0xF0004218U)
#define CAN_NCR1       TC1791_REG32(0xF0004300U)
#define CAN_NCR2       TC1791_REG32(0xF0004400U)
#define CAN_NCR3       TC1791_REG32(0xF0004500U)

#define CAN_MOFCR0     TC1791_REG32(0xF0005000U)
#define CAN_MOFGPR0    TC1791_REG32(0xF0005004U)
#define CAN_MOIPR0     TC1791_REG32(0xF0005008U)
#define CAN_MOAMR0     TC1791_REG32(0xF000500CU)
#define CAN_MOAR0      TC1791_REG32(0xF0005018U)
#define CAN_MOCTR0     TC1791_REG32(0xF000501CU)
#define CAN_MOFCR1     TC1791_REG32(0xF0005020U)
#define CAN_MOFGPR1    TC1791_REG32(0xF0005024U)
#define CAN_MOIPR1     TC1791_REG32(0xF0005028U)
#define CAN_MOAMR1     TC1791_REG32(0xF000502CU)
#define CAN_MODATAL1   TC1791_REG32(0xF0005030U)
#define CAN_MODATAH1   TC1791_REG32(0xF0005034U)
#define CAN_MOAR1      TC1791_REG32(0xF0005038U)
#define CAN_MOCTR1     TC1791_REG32(0xF000503CU)

#define P6_IOCR8       TC1791_REG32(0xF0001218U)
#define DMA_CLC        TC1791_REG32(0xF0003C00U)

#define SCU_PLLSTAT    (*(volatile SCU_PLLSTAT_t *)0xF0000514U)
#define SCU_PLLCON0    (*(volatile SCU_PLLCON0_t *)0xF0000518U)
#define SCU_PLLCON1    TC1791_REG32(0xF000051CU)
#define SCU_CCUCON0    TC1791_REG32(0xF0000530U)
#define SCU_RSTSTAT    TC1791_REG32(0xF0000550U)
#define WDT_CON0       (*(volatile WDT_CON0_t *)0xF00005F0U)
#define WDT_CON1       (*(volatile WDT_CON1_t *)0xF00005F4U)

#define PCP_CLC        TC1791_REG32(0xF0043F00U)
#define PCP_CS         TC1791_REG32(0xF0043F10U)
#define PCP_ICR        TC1791_REG32(0xF0043F20U)
#define PCP_ITR        TC1791_REG32(0xF0043F24U)
#define PCP_SRC8       TC1791_REG32(0xF0043FDCU)
#define PCP_SRC7       TC1791_REG32(0xF0043FE0U)
#define PCP_SRC6       TC1791_REG32(0xF0043FE4U)
#define PCP_SRC5       TC1791_REG32(0xF0043FE8U)
#define PCP_SRC4       TC1791_REG32(0xF0043FECU)

#define FLASH0_FSR     (*(volatile FLASHn_FSR_t *)0xF8002010U)
#define FLASH1_FSR     (*(volatile FLASHn_FSR_t *)0xF8004010U)

#endif /* TC1791_MINIMAL_H */
