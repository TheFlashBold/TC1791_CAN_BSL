/*
 * FLASH.h
 *
*/

#ifndef FLASH_H_
#define FLASH_H_

#include "MAIN.h"

enum FLASH_PROTECTION {
	READ_PROTECTION = 0x08,
	WRITE_PROTECTION = 0x05
};

void FLASH_eraseSector(volatile FLASHn_FSR_t *flashFSR, uword flashBaseAddress, uword sectorBaseAddress);
void FLASH_erasePhysSector(volatile FLASHn_FSR_t *flashFSR, uword flashBaseAddress, uword sectorBaseAddress);
void FLASH_sendPasswords(enum FLASH_PROTECTION whichProtection, uword flashBaseAddress, uword password0, uword password1, uword whichUCB);
void FLASH_writePage(volatile FLASHn_FSR_t *flashFSR, uword flashBaseAddress, uword pageBaseAddress, ubyte data[256]);
void FLASH_writePageDFlash(volatile FLASHn_FSR_t *flashFSR, uword flashBaseAddress, uword pageBaseAddress, ubyte data[128]);

#endif /* FLASH_H_ */
