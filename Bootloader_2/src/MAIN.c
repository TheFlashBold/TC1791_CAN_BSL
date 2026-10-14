//****************************************************************************
// @Project Includes
//****************************************************************************

#include "MAIN.h"
#include "lz4.h"

//****************************************************************************
// @Defines
//****************************************************************************

#define RESET_INDICATOR     ((SCU_RSTSTAT.reg & ((uword)(0x0001001B))))
#define WATCHDOG_RESET      ((uword)0x00000008)
#define SOFTWARE_RESET      ((uword)0x00000010)
#define ESR0_RESET          ((uword)0x00000001)
#define ESR1_RESET          ((uword)0x00000002)
#define POWERON_RESET       ((uword)0x00010000)

/*
 * TC1791 Data Flash (EEPROM) banks, non-cached, as read back with
 * compressed_read. Each PMU exposes one DFlash window of DFLASH_BANK_SIZE.
 */
#define DFLASH0_BASE 0xAF000000U
#define DFLASH1_BASE 0xAF080000U
#define DFLASH_BANK_SIZE 0x18000U
#define MAIN_IS_DFLASH_ADDRESS(addr) \
	(((addr) >= DFLASH0_BASE && (addr) < (DFLASH0_BASE + DFLASH_BANK_SIZE)) \
	|| ((addr) >= DFLASH1_BASE && (addr) < (DFLASH1_BASE + DFLASH_BANK_SIZE)))

//****************************************************************************
// @Global Variables
//****************************************************************************
volatile unsigned int DummyToForceRead;

//****************************************************************************
// @Function      void MAIN_vInit(void) 
//
//----------------------------------------------------------------------------
// @Description   This function initializes the microcontroller.
//
//----------------------------------------------------------------------------
// @Returnvalue   None
//
//----------------------------------------------------------------------------
// @Parameters    None
//
//----------------------------------------------------------------------------
// @Date          12/18/2020
//
//****************************************************************************

void MAIN_vInit(void) {
	///  -----------------------------------------------------------------------
	///  Clock System:
	///  -----------------------------------------------------------------------
	///  - external clock frequency: 20.00 MHz
	///  - input divider (PDIV): 2
	///  - PLL operation (VCOBYP = 0)
	///  - VCO range: 400 MHz - 500 MHz
	///  - feedback divider (NDIV): 40
	///  - the VCO output frequency is: 400.00 MHz
	///  - output divider (KDIV): 2
	///  - CPU clock: 200.00 MHz
	///  - the ratio fcpu /ffpi is  2 / 1
	///  - the ratio fcpu /fsri is  1 / 1
	///  - the ratio fcpu /fpcp is  1 / 1
	///  - system clock: 100.00 MHz

	/// Comparing with the Compiler settings
	if (((SCU_PLLCON0.reg & 0X1004E00) != 0X1004E00)
			|| ((SCU_PLLCON1.reg & 0X20001) != 0X20001)
			|| ((SCU_CCUCON0.reg & 0X1) != 0X1)) {

		//// - after a software reset PLL refuse to lock again unless oscillator is
		////   disconnected first

		MAIN_vResetENDINIT()
;		SCU_PLLCON0.bits.VCOBYP = 0;  // reset VCO bypass
		SCU_PLLCON0.bits.SETFINDIS = 1; // disconnect OSC from PLL
		MAIN_vSetENDINIT();

		if (!SCU_PLLSTAT.bits.PWDSTAT) {
			MAIN_vResetENDINIT()
;			SCU_CCUCON0.reg = 0x00000001; // set FPI,SRI and PCP dividers
			SCU_PLLCON0.bits.VCOBYP = 1; // set VCO bypass (goto Prescaler Mode)
			while (!SCU_PLLSTAT.bits.VCOBYST)
				; // wait for prescaler mode
			SCU_PLLCON0.reg = 0x01054E21; // set P,N divider, connect OSC
			SCU_PLLCON1.reg = 0x00020001; // set K1,K2 divider
			MAIN_vSetENDINIT();
			while (SCU_PLLSTAT.bits.VCOLOCK == 0)
				; // wait for LOCK
			MAIN_vResetENDINIT()
;			SCU_PLLCON0.bits.VCOBYP = 0; // Reset VCO bypass (Leave Prescaler Mode)
			MAIN_vSetENDINIT();
		}
	}

	MAIN_vResetENDINIT()
;	WDT_CON1.reg = 0X8; // Disable watchdog timer
	MAIN_vSetENDINIT();

	///  -----------------------------------------------------------------------
	///  Interrupt System:
	///  -----------------------------------------------------------------------
	///  - four arbitration cycles (max. 255 interrupt sources)
	///  - two clocks per arbitration cycle

	MTCR(0xFE2C, 0x00000000);      // load CPU interrupt control register
	ISYNC();

	///  -----------------------------------------------------------------------
	///  Peripheral Control Processor (PCP):
	///  -----------------------------------------------------------------------
	///  - stop the PCP internal clock when PCP is idle

	///  - use Full Context save area (R[0] - R[7])
	///  - start progam counter as left by last invocation
	///  - channel watchdog is disabled
	///  - maximum channel number checking is disabled

	MAIN_vResetENDINIT()
;	PCP_CLC.reg = 0x00000000;   // load PCP clock control register
	PCP_CS.reg = 0x00000200;   // load PCP control and status register
	MAIN_vSetENDINIT();

	///  - four arbitration cycles (max. 255 PCP channels)
	///  - two clocks per arbitration cycle
	PCP_ICR.reg = 0x00000000;   // load PCP interrupt control register

	///  - the PCP warning mechanism is disabled
	PCP_ITR.reg = 0x00000000;   // load PCP interrupt threshold register

	///  - type of service of PCP node 4 is CPU interrupt
	PCP_SRC4.reg = 0x00001000;   // load service request control register 4

	///  - type of service of PCP node 5 is CPU interrupt
	PCP_SRC5.reg = 0x00001000;   // load service request control register 5

	///  - type of service of PCP node 6 is CPU interrupt
	PCP_SRC6.reg = 0x00001000;   // load service request control register 6

	///  - type of service of PCP node 7 is CPU interrupt
	PCP_SRC7.reg = 0x00001000;   // load service request control register 7

	///  - type of service of PCP node 8 is CPU interrupt
	PCP_SRC8.reg = 0x00001000;   // load service request control register 8

	///  -----------------------------------------------------------------------
	///  Configuration of the DMA Module Clock:
	///  -----------------------------------------------------------------------
	///  - enable the DMA module

	MAIN_vResetENDINIT()
;	DMA_CLC.reg = 0x00000008;   // DMA clock control register
	DummyToForceRead = DMA_CLC.reg; // read it back to ensure it is read
	MAIN_vSetENDINIT();

	//   -----------------------------------------------------------------------
	//   Initialization of the Peripherals:
	//   -----------------------------------------------------------------------
	//   initializes the MultiCAN Controller
	CAN_vInit();

	///  -----------------------------------------------------------------------
	///  System Start Conditions:
	///  -----------------------------------------------------------------------

	//// - the CPU interrupt system is globally disabled
	DISABLE();
}

//****************************************************************************
// @Function      void MAIN_vWriteWDTCON0(uword uwValue) 
//
//----------------------------------------------------------------------------
// @Description   This function writes the parameter uwValue to the WDT_CON0 
//                register which is password protected. 
//
//----------------------------------------------------------------------------
// @Returnvalue   None
//
//----------------------------------------------------------------------------
// @Parameters    uwValue: 
//                Value for the WDTCON0 register
//

void MAIN_vWriteWDTCON0(uword uwValue) {
	uword uwDummy;

	uwDummy = WDT_CON0.reg;
	uwDummy |= 0x000000F0;       //  set HWPW1 = 1111b

	if (WDT_CON1.bits.DR) {
		uwDummy |= 0x00000008;     //  set HWPW0 = WDTDR
	} else {
		uwDummy &= ~0x00000008;     //  set HWPW0 = WDTDR
	}
	if (WDT_CON1.bits.IR) {
		uwDummy |= 0x00000004;     //  set HWPW0 = WDTIR
	} else {
		uwDummy &= ~0x00000004;     //  set HWPW0 = WDTIR
	}

	uwDummy &= ~0x00000002;       //  set WDTLCK = 0
	WDT_CON0.reg = uwDummy;          //  unlock access

	uwValue |= 0x000000F2;      //  set HWPW1 = 1111b and WDTLCK = 1
	uwValue &= ~0x0000000C;      //  set HWPW0 = 00b
	WDT_CON0.reg = uwValue;         //  write access and lock

}

struct bootloader_state boot;
struct password_cmd_state passwordCmdState;
struct write_page_state writePageState;
struct compress_read_state compressReadState;
struct compress_write_state compressWriteState;

void MAIN_sendCan(ubyte canData[]) {
	while (CAN_ubRequestMsgObj(1) != 1)
		;
	CAN_vLoadData(1, canData);
	CAN_vTransmit(1);
}

/*
 * 0x1: readDevice ID
 * Takes no arguments
 * Returns first 12 bytes of RAM, populated with Device ID from Configuration Sector by the BootROM.
 */
void MAIN_CMD_readDeviceId() {
	ubyte i;
	ubyte canData[8];
	ubyte *device_id = (ubyte *) 0xD0000000;
	canData[0] = 0x1;
	canData[1] = 0x0;
	for (i = 0; i < 6; i++) {
		canData[i + 2] = device_id[i];
	}
	MAIN_sendCan(canData);
	canData[1] = 0x1;
	for (i = 0; i < 6; i++) {
		canData[i + 2] = device_id[i + 6];
	}
	MAIN_sendCan(canData);
}

/*
 * 0x2: Read32.
 * Uses bytes 1-4 to set the address to read from.
 * Returns the value from performing a direct Tricore bus read to the read address. This can be RAM, a register, or an address in Flash if it is unlocked and in read mode.
 */
void MAIN_CMD_read32(CAN_SWObj *cur_msg) {
	uword address = (cur_msg->ubData[1] << 24) | (cur_msg->ubData[2] << 16)
			| (cur_msg->ubData[3] << 8) | cur_msg->ubData[4];
	uword address_value = *(uword *) address;
	ubyte canData[8];
	canData[0] = 0x2;
	canData[4] = (address_value >> 24) & 0xFF;
	canData[3] = (address_value >> 16) & 0xFF;
	canData[2] = (address_value >> 8) & 0xFF;
	canData[1] = address_value & 0xFF;
	canData[5] = 0xFF;
	canData[6] = 0xFF;
	canData[7] = 0xFF;
	MAIN_sendCan(canData);
}

/*
 * 0x3 : write32SetAddress
 * Uses bytes 1-4 to set the address pointer for a Write32 command.
 * Expects a subsequent 0x4 command with the data.
 * Places the read loop in a wait state to wait for the subsequent 0x4 command.
 */
void MAIN_CMD_write32SetAddress(CAN_SWObj *cur_msg) {
	uword address = (cur_msg->ubData[1] << 24) | (cur_msg->ubData[2] << 16)
			| (cur_msg->ubData[3] << 8) | cur_msg->ubData[4];
	ubyte canData[8];
	canData[0] = 0x3;
	canData[4] = (address >> 24) & 0xFF;
	canData[3] = (address >> 16) & 0xFF;
	canData[2] = (address >> 8) & 0xFF;
	canData[1] = address & 0xFF;
	canData[5] = 0xFF;
	canData[6] = 0xFF;
	canData[7] = 0xFF;
	MAIN_sendCan(canData);
	boot.address = address;
}
/*
 * 0x3 : write32SetData
 * Uses bytes 1-4 to set the data value for a Write32 command, and performs the write
 * Only called when read loop was already in a wait state to wait for the subsequent 0x4 command after write32SetAddress
 */
void MAIN_CMD_write32SetData(CAN_SWObj *cur_msg) {
	uword data = (cur_msg->ubData[1] << 24) | (cur_msg->ubData[2] << 16)
			| (cur_msg->ubData[3] << 8) | cur_msg->ubData[4];
	*(uword *) boot.address = data;
	ubyte canData[8];
	canData[0] = 0x3;
	canData[4] = (data >> 24) & 0xFF;
	canData[3] = (data >> 16) & 0xFF;
	canData[2] = (data >> 8) & 0xFF;
	canData[1] = data & 0xFF;
	canData[5] = 0xFF;
	canData[6] = 0xFF;
	canData[7] = 0xFF;
	MAIN_sendCan(canData);
}

/*
 * 0x4: unlockPasswordSetFirstPassword
 * Uses bytes 1-4 to set the first flash password value.
 * Uses byte 5 to set the "read" or "write" mode to be unlocked.
 * Uses byte 6 to set which UCB block the passwords are for.
 * Uses byte 7 to specify which flash controller to use.
 * Places the read loop in a wait state to wait for the next 0x5 command containing the second password.
 */
void MAIN_CMD_unlockPasswordSetFirstPassword(CAN_SWObj *cur_msg) {
	uword firstPassword = (cur_msg->ubData[1] << 24)
			| (cur_msg->ubData[2] << 16) | (cur_msg->ubData[3] << 8)
			| cur_msg->ubData[4];
	ubyte readOrWrite = cur_msg->ubData[5];
	ubyte whichUCB = cur_msg->ubData[6];
	ubyte whichController = cur_msg->ubData[7];
	ubyte canData[8];
	canData[0] = 0x4;
	canData[4] = (firstPassword >> 24) & 0xFF;
	canData[3] = (firstPassword >> 16) & 0xFF;
	canData[2] = (firstPassword >> 8) & 0xFF;
	canData[1] = firstPassword & 0xFF;
	canData[5] = 0xFF;
	canData[6] = 0xFF;
	canData[7] = 0xFF;
	MAIN_sendCan(canData);
	passwordCmdState.whichUCB = whichUCB;
	passwordCmdState.readOrWrite = readOrWrite;
	passwordCmdState.firstPassword = firstPassword;
	passwordCmdState.whichController = whichController;
}

/*
 * 0x4: unlockPassword
 * Uses bytes 1-4 to set the second flash password value.
 * Performs the unlock procedure.
 * Returns the flash status register content.
 */
void MAIN_CMD_unlockPassword(CAN_SWObj *cur_msg) {
	uword secondPassword = (cur_msg->ubData[1] << 24)
			| (cur_msg->ubData[2] << 16) | (cur_msg->ubData[3] << 8)
			| cur_msg->ubData[4];
	uword baseAddress =
			(passwordCmdState.whichController > 0) ? 0x80800000 : 0x80000000;
	FLASH_sendPasswords(passwordCmdState.readOrWrite, baseAddress,
			passwordCmdState.firstPassword, secondPassword,
			passwordCmdState.whichUCB);
	volatile FLASHn_FSR_t *flashFSR =
			(passwordCmdState.whichController > 0) ? &FLASH1_FSR : &FLASH0_FSR;
	uword flashStatus = flashFSR->reg;
	ubyte canData[8];
	canData[0] = 0x4;
	canData[4] = (flashStatus >> 24) & 0xFF;
	canData[3] = (flashStatus >> 16) & 0xFF;
	canData[2] = (flashStatus >> 8) & 0xFF;
	canData[1] = flashStatus & 0xFF;
	canData[5] = 0xFF;
	canData[6] = 0xFF;
	canData[7] = 0xFF;
	MAIN_sendCan(canData);
}

/*
 * 0x5: eraseFlashSector
 * Uses bytes 1-4 to set the address to erase.
 * Erases the sector that contains the address. Program Flash sectors use the
 * Erase Sector command; Data Flash (EEPROM) sectors use Erase Phys Sector,
 * because the plain Erase Sector command is rejected for DFlash. The two
 * DFlash banks are whole sectors erased completely: DS0 at 0xAF000000 and
 * DS1 at 0xAF080000. Returns the flash status register content.
 */
void MAIN_CMD_eraseFlashSector(CAN_SWObj *cur_msg) {
	uword address = (cur_msg->ubData[1] << 24) | (cur_msg->ubData[2] << 16)
			| (cur_msg->ubData[3] << 8) | cur_msg->ubData[4];
	ubyte isDFlash = MAIN_IS_DFLASH_ADDRESS(address);
	/*
	 * Command cycles use the DFlash bank base; PFlash uses its per-PMU bank
	 * base. Both DFlash banks report in FLASH0_FSR (D0BUSY/D1BUSY), so DFlash
	 * always uses FLASH0; PFlash uses its per-PMU register. Verified against
	 * the Simos18 ASW flash driver (FUN_80881538, FUN_8088092c).
	 */
	uword baseAddress = isDFlash
			? ((address >= DFLASH1_BASE) ? DFLASH1_BASE : DFLASH0_BASE)
			: ((address >= 0x80800000U) ? 0x80800000U : 0x80000000U);
	volatile FLASHn_FSR_t *flashFSR = isDFlash
			? &FLASH0_FSR
			: ((address >= 0x80800000U) ? &FLASH1_FSR : &FLASH0_FSR);
	if (isDFlash) {
		FLASH_erasePhysSector(flashFSR, baseAddress, address);
	} else {
		FLASH_eraseSector(flashFSR, baseAddress, address);
	}
	uword flashStatus = flashFSR->reg;
	ubyte canData[8];
	canData[0] = 0x5;
	canData[4] = (flashStatus >> 24) & 0xFF;
	canData[3] = (flashStatus >> 16) & 0xFF;
	canData[2] = (flashStatus >> 8) & 0xFF;
	canData[1] = flashStatus & 0xFF;
	canData[5] = 0xFF;
	canData[6] = 0xFF;
	canData[7] = 0xFF;
	MAIN_sendCan(canData);
}

/*
 * 0x5: writeFlashPage
 * Uses bytes 1-4 to set the address to be written.
 * Configures the state of the BSL to await 256 bytes of page data to write to the Page.
 * Returns the address.
 */
void MAIN_CMD_writeFlashPage(CAN_SWObj *cur_msg) {
	uword address = (cur_msg->ubData[1] << 24) | (cur_msg->ubData[2] << 16)
			| (cur_msg->ubData[3] << 8) | cur_msg->ubData[4];
	writePageState.address = address;
	writePageState.cursor = 0;
	ubyte canData[8];
	canData[0] = 0x6;
	canData[4] = (address >> 24) & 0xFF;
	canData[3] = (address >> 16) & 0xFF;
	canData[2] = (address >> 8) & 0xFF;
	canData[1] = address & 0xFF;
	canData[5] = 0xFF;
	canData[6] = 0xFF;
	canData[7] = 0xFF;
	MAIN_sendCan(canData);
}

ubyte MAIN_CMD_writeFlashPageData(CAN_SWObj *cur_msg) {
	int i = 1;
	for (i = 1; i < 8; i++) {
		writePageState.assemblyBuffer[writePageState.cursor] =
				cur_msg->ubData[i];
		if (writePageState.cursor == 255) {
			break;
		}
		writePageState.cursor += 1;
	}
	if (writePageState.cursor == 255) {
		uword baseAddress =
				(writePageState.address >= 0x80800000) ?
						0x80800000 : 0x80000000;
		uword whichController = (writePageState.address >= 0x80800000);
		volatile FLASHn_FSR_t *flashFSR =
				(whichController > 0) ? &FLASH1_FSR : &FLASH0_FSR;
		FLASH_writePage(flashFSR, baseAddress, writePageState.address,
				writePageState.assemblyBuffer);
		ubyte canData[8];
		canData[0] = 0x6;
		canData[4] = 0x00;
		canData[3] = 0x00;
		canData[2] = 0x00;
		canData[1] = 0x00;
		canData[5] = 0xFF;
		canData[6] = 0xFF;
		canData[7] = 0xFF;
		MAIN_sendCan(canData);
		return 1;
	}
	return 0;
}

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void MAIN_waitAckOrTimeout(uword timeout_ms) {
	CAN_SWObj cur_msg;
	uword start_val = STM_TIM0.reg;
	while (1) {
		if (CAN_ubNewData(0)) {
			CAN_vGetMsgObj(0, &cur_msg);
			CAN_vReleaseObj(0);
			if(cur_msg.ubData[0] == 0x7 && cur_msg.ubData[1] == 0xAC) {
				return;
			}
		} else {
			if((STM_TIM0.reg - start_val) > (timeout_ms * 100000)) {
				return;
			}
		}
	}
}

void MAIN_CMD_readCompressed(CAN_SWObj *cur_msg) {
	uword address = (cur_msg->ubData[1] << 24) | (cur_msg->ubData[2] << 16)
			| (cur_msg->ubData[3] << 8) | cur_msg->ubData[4];
	uword length = (cur_msg->ubData[5] << 16) | (cur_msg->ubData[6] << 8)
			| cur_msg->ubData[7];
	compressReadState.address = address;
	compressReadState.length = length;
	compressReadState.cursor = 0;

	for (;;) {
		const char* inpPtr = (const char*) (compressReadState.address
				+ compressReadState.cursor);
		const int inpBytes = MIN(4096,
				compressReadState.length - compressReadState.cursor);
		compressReadState.cursor += inpBytes;
		if (0 == inpBytes) {
			break;
		}
		{
			char cmpBuf[LZ4_COMPRESSBOUND(4096)];
			const int cmpBytes = LZ4_compress_default(inpPtr, cmpBuf, inpBytes,
					LZ4_COMPRESSBOUND(4096));
			if (cmpBytes <= 0) {
				break;
			}
			ubyte canData[8];
			canData[0] = 0x7;
			canData[1] = ((uword) inpPtr >> 24) & 0xFF;
			canData[2] = ((uword) inpPtr >> 16) & 0xFF;
			canData[3] = ((uword) inpPtr >> 8) & 0xFF;
			canData[4] = (uword) inpPtr & 0xFF;
			canData[5] = (cmpBytes >> 16) & 0xFF;
			canData[6] = (cmpBytes >> 8) & 0xFF;
			canData[7] = cmpBytes & 0xFF;
			MAIN_sendCan(canData);
			ubyte canSeq = 0;
			int i = 0;
			for (i = 0; i < cmpBytes; i += 6) {
				canSeq++;
				canData[0] = 0x7;
				canData[1] = canSeq;
				canData[2] = cmpBuf[i];
				canData[3] = cmpBuf[i + 1];
				canData[4] = cmpBuf[i + 2];
				canData[5] = cmpBuf[i + 3];
				canData[6] = cmpBuf[i + 4];
				canData[7] = cmpBuf[i + 5];
				MAIN_sendCan(canData);
			}
			MAIN_waitAckOrTimeout(10000U);
		}
	}
}

#define COMPRESSED_WRITE_READY 0xA0
#define COMPRESSED_WRITE_BLOCK_READY 0xA1
#define COMPRESSED_WRITE_BLOCK_DONE 0xA2

#define COMPRESSED_WRITE_ERR_RANGE 0x01
#define COMPRESSED_WRITE_ERR_META 0x02
#define COMPRESSED_WRITE_ERR_SEQUENCE 0x03
#define COMPRESSED_WRITE_ERR_DECOMPRESS 0x04
#define COMPRESSED_WRITE_ERR_FLASH 0x05
#define COMPRESSED_WRITE_ERR_STATE 0x06

/* PFlash FSR error bits: PFOPER(8), SQER(10), PROER(11), PFSBER(12). */
#define FLASH_STATUS_ERROR_MASK 0x00001D00U
/* DFlash FSR error bits: DFOPER(9), SQER(10), PROER(11). */
#define FLASH_STATUS_ERROR_MASK_DFLASH 0x00000E00U

void MAIN_sendCompressedWriteResponse(ubyte stage, ubyte error, ubyte final,
		uword flashStatus) {
	ubyte canData[8];
	canData[0] = WRITE_COMPRESSED;
	canData[1] = stage;
	canData[2] = error;
	canData[3] = final;
	canData[4] = (flashStatus >> 24) & 0xFF;
	canData[5] = (flashStatus >> 16) & 0xFF;
	canData[6] = (flashStatus >> 8) & 0xFF;
	canData[7] = flashStatus & 0xFF;
	MAIN_sendCan(canData);
}

void MAIN_abortCompressedWrite(ubyte stage, ubyte error, uword flashStatus) {
	boot.status = WAIT_COMMAND;
	memset(&compressWriteState, 0x00, sizeof(struct compress_write_state));
	MAIN_sendCompressedWriteResponse(stage, error, 0, flashStatus);
}

/*
 * Returns the flash page size (256 for PFlash, 128 for DFlash/EEPROM) for a
 * valid compressed-write range, or 0 if the range is not writable. The address
 * and length must be aligned to the page size of the targeted array and the
 * whole range must stay inside a single PFlash bank or DFlash bank.
 */
uword MAIN_compressedWritePageSize(uword address, uword length) {
	if (length == 0U) {
		return 0U;
	}
	/* Program Flash: 256-byte pages. */
	if ((address & (COMPRESSED_WRITE_PFLASH_PAGE_SIZE - 1U)) == 0U
			&& (length & (COMPRESSED_WRITE_PFLASH_PAGE_SIZE - 1U)) == 0U) {
		if (address >= 0x80000000U && address < 0x80200000U
				&& length <= (0x80200000U - address)) {
			return COMPRESSED_WRITE_PFLASH_PAGE_SIZE;
		}
		if (address >= 0x80800000U && address < 0x80A00000U
				&& length <= (0x80A00000U - address)) {
			return COMPRESSED_WRITE_PFLASH_PAGE_SIZE;
		}
	}
	/* Data Flash / EEPROM: 128-byte pages. */
	if ((address & (COMPRESSED_WRITE_DFLASH_PAGE_SIZE - 1U)) == 0U
			&& (length & (COMPRESSED_WRITE_DFLASH_PAGE_SIZE - 1U)) == 0U) {
		if (address >= DFLASH0_BASE
				&& address < (DFLASH0_BASE + DFLASH_BANK_SIZE)
				&& length <= ((DFLASH0_BASE + DFLASH_BANK_SIZE) - address)) {
			return COMPRESSED_WRITE_DFLASH_PAGE_SIZE;
		}
		if (address >= DFLASH1_BASE
				&& address < (DFLASH1_BASE + DFLASH_BANK_SIZE)
				&& length <= ((DFLASH1_BASE + DFLASH_BANK_SIZE) - address)) {
			return COMPRESSED_WRITE_DFLASH_PAGE_SIZE;
		}
	}
	return 0U;
}

/*
 * 0x8: Begin an LZ4-compressed flash write.
 * Bytes 1-4 contain the big-endian destination address and bytes 5-7 the
 * 24-bit uncompressed length. The complete range must stay inside a single
 * TC1791 PFlash bank (256-byte pages) or DFlash/EEPROM bank (128-byte pages),
 * aligned to that array's page size. Flash must already be erased.
 */
void MAIN_CMD_writeCompressedStart(CAN_SWObj *cur_msg) {
	uword address = (cur_msg->ubData[1] << 24) | (cur_msg->ubData[2] << 16)
			| (cur_msg->ubData[3] << 8) | cur_msg->ubData[4];
	uword length = (cur_msg->ubData[5] << 16) | (cur_msg->ubData[6] << 8)
			| cur_msg->ubData[7];
	uword pageSize = MAIN_compressedWritePageSize(address, length);
	if (pageSize == 0U) {
		MAIN_abortCompressedWrite(COMPRESSED_WRITE_READY,
				COMPRESSED_WRITE_ERR_RANGE, 0);
		return;
	}
	memset(&compressWriteState, 0x00, sizeof(struct compress_write_state));
	compressWriteState.address = address;
	compressWriteState.remaining = length;
	compressWriteState.pageSize = pageSize;
	boot.status = WAIT_COMPRESSED_WRITE_META;
	MAIN_sendCompressedWriteResponse(COMPRESSED_WRITE_READY, 0, 0, 0);
}

/*
 * Block metadata: 08 FF <raw_len:BE16> <compressed_len:BE16> 00 00.
 * A raw block is at most 4096 bytes and must contain whole flash pages.
 */
void MAIN_CMD_writeCompressedMeta(CAN_SWObj *cur_msg) {
	uword rawLength;
	uword compressedLength;
	if (cur_msg->ubData[0] != WRITE_COMPRESSED || cur_msg->ubData[1] != 0xFF) {
		MAIN_abortCompressedWrite(COMPRESSED_WRITE_BLOCK_READY,
				COMPRESSED_WRITE_ERR_STATE, 0);
		return;
	}
	rawLength = (cur_msg->ubData[2] << 8) | cur_msg->ubData[3];
	compressedLength = (cur_msg->ubData[4] << 8) | cur_msg->ubData[5];
	if (rawLength == 0U || rawLength > COMPRESSED_WRITE_BLOCK_SIZE
			|| (rawLength & (compressWriteState.pageSize - 1U)) != 0U
			|| rawLength > compressWriteState.remaining
			|| compressedLength == 0U
			|| compressedLength > COMPRESSED_WRITE_BUFFER_SIZE) {
		MAIN_abortCompressedWrite(COMPRESSED_WRITE_BLOCK_READY,
				COMPRESSED_WRITE_ERR_META, 0);
		return;
	}
	compressWriteState.rawLength = rawLength;
	compressWriteState.compressedLength = compressedLength;
	compressWriteState.compressedCursor = 0;
	compressWriteState.expectedSequence = 0;
	boot.status = WAIT_COMPRESSED_WRITE_DATA;
	MAIN_sendCompressedWriteResponse(COMPRESSED_WRITE_BLOCK_READY, 0, 0, 0);
}

void MAIN_CMD_finishCompressedWriteBlock(void) {
	int decompressedLength;
	uword pageOffset;
	ubyte isDFlash = (compressWriteState.pageSize
			== COMPRESSED_WRITE_DFLASH_PAGE_SIZE);
	/*
	 * PFlash uses the per-PMU status register (PMU0 below 0x80800000, PMU1
	 * at-or-above). Both DFlash banks (DS0 at 0xAF000000, DS1 at 0xAF080000)
	 * report in FLASH0_FSR via D0BUSY / D1BUSY, so DFlash always uses FLASH0.
	 * Verified against the Simos18 ASW flash driver (FUN_8088092c, FUN_80881454).
	 */
	volatile FLASHn_FSR_t *flashFSR = isDFlash
			? &FLASH0_FSR
			: ((compressWriteState.address >= 0x80800000U) ? &FLASH1_FSR : &FLASH0_FSR);
	uword pflashBase = (compressWriteState.address >= 0x80800000U) ?
			0x80800000U : 0x80000000U;
	/*
	 * Command cycles use the DFlash bank base; the bank is selected by this
	 * base, not by A16 within it. Matches the ASW base resolver at 0x80881538.
	 */
	uword dflashBase = (compressWriteState.address >= DFLASH1_BASE) ?
			DFLASH1_BASE : DFLASH0_BASE;
	uword errorMask = isDFlash ?
			FLASH_STATUS_ERROR_MASK_DFLASH : FLASH_STATUS_ERROR_MASK;
	uword flashStatus;
	ubyte final;

	decompressedLength = LZ4_decompress_safe(
			(const char*) compressWriteState.compressedBuffer,
			(char*) compressWriteState.decompressedBuffer,
			compressWriteState.compressedLength, compressWriteState.rawLength);
	if (decompressedLength != (int) compressWriteState.rawLength) {
		MAIN_abortCompressedWrite(COMPRESSED_WRITE_BLOCK_DONE,
				COMPRESSED_WRITE_ERR_DECOMPRESS, 0);
		return;
	}

	for (pageOffset = 0; pageOffset < compressWriteState.rawLength;
			pageOffset += compressWriteState.pageSize) {
		uword pageAddress = compressWriteState.address + pageOffset;
		if (isDFlash) {
			FLASH_writePageDFlash(flashFSR, dflashBase, pageAddress,
					&compressWriteState.decompressedBuffer[pageOffset]);
		} else {
			FLASH_writePage(flashFSR, pflashBase, pageAddress,
					&compressWriteState.decompressedBuffer[pageOffset]);
		}
		flashStatus = flashFSR->reg;
		if ((flashStatus & errorMask) != 0U) {
			MAIN_abortCompressedWrite(COMPRESSED_WRITE_BLOCK_DONE,
					COMPRESSED_WRITE_ERR_FLASH, flashStatus);
			return;
		}
	}

	compressWriteState.address += compressWriteState.rawLength;
	compressWriteState.remaining -= compressWriteState.rawLength;
	final = (compressWriteState.remaining == 0U);
	flashStatus = flashFSR->reg;
	boot.status = final ? WAIT_COMMAND : WAIT_COMPRESSED_WRITE_META;
	MAIN_sendCompressedWriteResponse(COMPRESSED_WRITE_BLOCK_DONE, 0, final,
			flashStatus);
	if (final) {
		memset(&compressWriteState, 0x00, sizeof(struct compress_write_state));
	}
}

/* Data frame: 08 <sequence> <six compressed bytes>. */
void MAIN_CMD_writeCompressedData(CAN_SWObj *cur_msg) {
	uword bytesRemaining;
	uword bytesToCopy;
	uword i;
	if (cur_msg->ubData[0] != WRITE_COMPRESSED
			|| cur_msg->ubData[1] != compressWriteState.expectedSequence) {
		MAIN_abortCompressedWrite(COMPRESSED_WRITE_BLOCK_DONE,
				COMPRESSED_WRITE_ERR_SEQUENCE, 0);
		return;
	}
	bytesRemaining = compressWriteState.compressedLength
			- compressWriteState.compressedCursor;
	bytesToCopy = MIN(6U, bytesRemaining);
	for (i = 0; i < bytesToCopy; i++) {
		compressWriteState.compressedBuffer[compressWriteState.compressedCursor]
				= cur_msg->ubData[i + 2];
		compressWriteState.compressedCursor++;
	}
	compressWriteState.expectedSequence++;
	if (compressWriteState.compressedCursor
			== compressWriteState.compressedLength) {
		MAIN_CMD_finishCompressedWriteBlock();
	}
}

void MAIN_processMessage(CAN_SWObj *cur_msg) {
	switch (boot.status) {
	case WAIT_COMMAND: {
		boot.command = cur_msg->ubData[0];
		switch (boot.command) {
		case READ_DEVICEID: // Read Device ID
		{
			MAIN_CMD_readDeviceId();
			break;
		}
		case READ_MEM32: // Read32 from Arbitrary Address
		{
			MAIN_CMD_read32(cur_msg);
			break;
		}
		case WRITE_MEM32: // Write32 to Arbitrary Address
		{
			MAIN_CMD_write32SetAddress(cur_msg);
			boot.status = WAIT_WRITE32_DATA;
			break;
		}
		case UNLOCK_PASSWORD: // Send UNLOCK command to Flash controller
			MAIN_CMD_unlockPasswordSetFirstPassword(cur_msg);
			boot.status = WAIT_PASSWORD_DATA;
			break;
		case ERASE_SECTOR:
			MAIN_CMD_eraseFlashSector(cur_msg);
			break;
		case WRITE_PAGE:
			MAIN_CMD_writeFlashPage(cur_msg);
			boot.status = WAIT_WRITEPAGE_DATA;
			break;
		case READ_COMPRESSED:
			MAIN_CMD_readCompressed(cur_msg);
			break;
		case WRITE_COMPRESSED:
			MAIN_CMD_writeCompressedStart(cur_msg);
			break;
		default: {
			break;
		}
		}
		break;
	}
	case WAIT_WRITE32_DATA: {
		boot.command = cur_msg->ubData[0];
		switch (boot.command) {
		case WRITE_MEM32: {
			MAIN_CMD_write32SetData(cur_msg);
			break;
		}
		default: {
			ubyte canData[8];
			canData[0] = 0x4F;
			canData[4] = 0xFF;
			canData[3] = 0xFF;
			canData[2] = 0xFF;
			canData[1] = 0xFF;
			canData[5] = 0xFF;
			canData[6] = 0xFF;
			canData[7] = 0xFF;
			MAIN_sendCan(canData);
			break;
		}
		}
		boot.address = 0;
		boot.status = WAIT_COMMAND;
		break;
	}
	case WAIT_PASSWORD_DATA: {
		boot.command = cur_msg->ubData[0];
		switch (boot.command) {
		case UNLOCK_PASSWORD: {
			MAIN_CMD_unlockPassword(cur_msg);
			break;
		}
		default: {
			ubyte canData[8];
			canData[0] = 0x5F;
			canData[4] = 0xFF;
			canData[3] = 0xFF;
			canData[2] = 0xFF;
			canData[1] = 0xFF;
			canData[5] = 0xFF;
			canData[6] = 0xFF;
			canData[7] = 0xFF;
			MAIN_sendCan(canData);
			break;
		}
		}
		boot.address = 0;
		boot.status = WAIT_COMMAND;
		break;
	}
	case WAIT_WRITEPAGE_DATA: {
		boot.command = cur_msg->ubData[0];
		switch (boot.command) {
		case WRITE_PAGE: {
			if (MAIN_CMD_writeFlashPageData(cur_msg) > 0) {
				boot.status = WAIT_COMMAND;
			}
			break;
		}
		default: {
			boot.status = WAIT_COMMAND;
			ubyte canData[8];
			canData[0] = 0x6F;
			canData[4] = 0xFF;
			canData[3] = 0xFF;
			canData[2] = 0xFF;
			canData[1] = 0xFF;
			canData[5] = 0xFF;
			canData[6] = 0xFF;
			canData[7] = 0xFF;
			MAIN_sendCan(canData);
			break;
		}
		}
		break;
	}
	case WAIT_COMPRESSED_WRITE_META: {
		MAIN_CMD_writeCompressedMeta(cur_msg);
		break;
	}
	case WAIT_COMPRESSED_WRITE_DATA: {
		MAIN_CMD_writeCompressedData(cur_msg);
		break;
	}
	default: {
		break;
	}
	}
}

sword main(void) {

	MAIN_vInit();
	CAN_SWObj cur_msg;
	memset(&boot, 0x00, sizeof(struct bootloader_state));
	do {
		if (CAN_ubNewData(0)) { // check for new message
			CAN_vGetMsgObj(0, &cur_msg); // copy and free to allow next msg into slot
			CAN_vReleaseObj(0);
			MAIN_processMessage(&cur_msg);
		}
	} while (1);
} //  End of function main
