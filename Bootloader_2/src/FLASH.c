/*
 * FLASH.c
 *
 *  Created on: Dec 29, 2020
 *      Author: b l
 */

#include "FLASH.h"

void FLASH_sendPasswords(enum FLASH_PROTECTION whichProtection, uword flashBaseAddress, uword password0, uword password1, uword whichUCB)
{
	// Reset flash controller status
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00F5;

	// Send passwords to flash controller
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00AA;
	(*(volatile uword *) (flashBaseAddress + 0xAAA8)) = 0x0055;
	(*(volatile uword *) (flashBaseAddress + 0x553C)) = whichUCB;
	(*(volatile uword *) (flashBaseAddress + 0xAAA8)) = password0;
	(*(volatile uword *) (flashBaseAddress + 0xAAA8)) = password1;
	(*(volatile uword *) (flashBaseAddress + 0x5558)) = whichProtection;

	__asm("nop");
	__asm("nop");
	__asm("nop");
}

void FLASH_eraseSector(volatile FLASHn_FSR_t *flashFSR, uword flashBaseAddress, uword sectorBaseAddress)
{
	// Reset flash controller status
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00F5;

	// Erase Sector
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00AA;
	(*(volatile uword *) (flashBaseAddress + 0xAAA8)) = 0x0055;
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x0080;
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00AA;
	(*(volatile uword *) (flashBaseAddress + 0xAAA8)) = 0x0055;
	(*(volatile uword *) (sectorBaseAddress)) = 0x0030;
	// Wait for FSR to become busy (valid)
	while ((flashFSR->bits.PBUSY == 0) && (flashFSR->bits.D0BUSY == 0) && (flashFSR->bits.D1BUSY == 0));
	// Wait for FSR to finish operation
	while ((flashFSR->bits.PBUSY != 0) || (flashFSR->bits.D0BUSY != 0) || (flashFSR->bits.D1BUSY != 0));
}

void FLASH_erasePhysSector(volatile FLASHn_FSR_t *flashFSR, uword flashBaseAddress, uword sectorBaseAddress)
{
	// Erase Phys Sector. Identical six-cycle sequence to Erase Sector but with
	// a 0x40 confirm pattern instead of 0x30. This is the only erase that works
	// on Data Flash sectors (the plain Erase Sector command is rejected for
	// DFlash). See TC1797 PMU, Table 5-4 and Section 5.6.3.4.

	// Reset flash controller status
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00F5;

	// Erase Physical Sector
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00AA;
	(*(volatile uword *) (flashBaseAddress + 0xAAA8)) = 0x0055;
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x0080;
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00AA;
	(*(volatile uword *) (flashBaseAddress + 0xAAA8)) = 0x0055;
	(*(volatile uword *) (sectorBaseAddress)) = 0x0040;
	// Wait for FSR to become busy (valid)
	while ((flashFSR->bits.PBUSY == 0) && (flashFSR->bits.D0BUSY == 0) && (flashFSR->bits.D1BUSY == 0));
	// Wait for FSR to finish operation
	while ((flashFSR->bits.PBUSY != 0) || (flashFSR->bits.D0BUSY != 0) || (flashFSR->bits.D1BUSY != 0));
}

void FLASH_writePage(volatile FLASHn_FSR_t *flashFSR, uword flashBaseAddress, uword pageBaseAddress, ubyte dataArray[256]) {
	// Reset flash controller status
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00F5;
	// Enter Assemble Page Mode
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x0050;
	// Load Page Data 64 bits (8 bytes) at a time.
	unsigned int i;
	unsigned long long data = 0;
	for (i = 0; i < 256; i += 8) {
		data = (unsigned long long)dataArray[i] & 0xFF;
		data += (unsigned long long)(dataArray[i + 1] & 0xFF) << 8;
		data += (unsigned long long)(dataArray[i + 2] & 0xFF) << 16;
		data += (unsigned long long)(dataArray[i + 3] & 0xFF) << 24;
		data += (unsigned long long)(dataArray[i + 4] & 0xFF) << 32;
		data += (unsigned long long)(dataArray[i + 5] & 0xFF) << 40;
		data += (unsigned long long)(dataArray[i + 6] & 0xFF) << 48;
		data += (unsigned long long)(dataArray[i + 7] & 0xFF) << 56;
		(*(unsigned long long*) (flashBaseAddress + 0x55F0)) = data;
	}

	// Send Write Page Command
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00AA;
	(*(volatile uword *) (flashBaseAddress + 0xAAA8)) = 0x0055;
	(*(volatile unsigned long *) (flashBaseAddress + 0x5554)) = 0xA0;
	(*(volatile unsigned long *) (pageBaseAddress)) = 0x00AA;

	// Wait for busy state to set
	while ((flashFSR->bits.PBUSY == 0) && (flashFSR->bits.D0BUSY == 0) && (flashFSR->bits.D1BUSY == 0));
	// Now wait for busy state to clear (FSR to finish operation)
	while ((flashFSR->bits.PBUSY != 0) || (flashFSR->bits.D0BUSY != 0) || (flashFSR->bits.D1BUSY != 0));
}

void FLASH_writePageDFlash(volatile FLASHn_FSR_t *flashFSR, uword flashBaseAddress, uword pageBaseAddress, ubyte dataArray[128]) {
	// Data Flash pages are 128 bytes (16 double-words) instead of 256, and the
	// Enter Page Mode command selects the Data Flash array with "y" = D (0x5D)
	// rather than 0x50 for Program Flash. All command cycles must use a base
	// address whose A31-A16 bits point at the addressed DFlash bank; the caller
	// passes that bank base in flashBaseAddress. See TC1797 PMU, Table 5-4.

	// Reset flash controller status
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00F5;
	// Enter Assemble Page Mode for the Data Flash (data = 0x5y, y = D)
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x005D;
	// Load Page Data 64 bits (8 bytes) at a time; 128 bytes = 16 double-words.
	unsigned int i;
	unsigned long long data = 0;
	for (i = 0; i < 128; i += 8) {
		data = (unsigned long long)dataArray[i] & 0xFF;
		data += (unsigned long long)(dataArray[i + 1] & 0xFF) << 8;
		data += (unsigned long long)(dataArray[i + 2] & 0xFF) << 16;
		data += (unsigned long long)(dataArray[i + 3] & 0xFF) << 24;
		data += (unsigned long long)(dataArray[i + 4] & 0xFF) << 32;
		data += (unsigned long long)(dataArray[i + 5] & 0xFF) << 40;
		data += (unsigned long long)(dataArray[i + 6] & 0xFF) << 48;
		data += (unsigned long long)(dataArray[i + 7] & 0xFF) << 56;
		(*(unsigned long long*) (flashBaseAddress + 0x55F0)) = data;
	}

	// Send Write Page Command (identical sequence to Program Flash)
	(*(volatile uword *) (flashBaseAddress + 0x5554)) = 0x00AA;
	(*(volatile uword *) (flashBaseAddress + 0xAAA8)) = 0x0055;
	(*(volatile unsigned long *) (flashBaseAddress + 0x5554)) = 0xA0;
	(*(volatile unsigned long *) (pageBaseAddress)) = 0x00AA;

	// Wait for busy state to set
	while ((flashFSR->bits.PBUSY == 0) && (flashFSR->bits.D0BUSY == 0) && (flashFSR->bits.D1BUSY == 0));
	// Now wait for busy state to clear (FSR to finish operation)
	while ((flashFSR->bits.PBUSY != 0) || (flashFSR->bits.D0BUSY != 0) || (flashFSR->bits.D1BUSY != 0));
}
