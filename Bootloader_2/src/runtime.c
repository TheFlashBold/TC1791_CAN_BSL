/* Minimal freestanding runtime required by the bundled LZ4 implementation. */

#include <stddef.h>

void *memcpy(void *destination, const void *source, size_t length)
{
	unsigned char *destinationBytes = (unsigned char *)destination;
	const unsigned char *sourceBytes = (const unsigned char *)source;
	size_t index;

	for (index = 0; index < length; index++) {
		destinationBytes[index] = sourceBytes[index];
	}
	return destination;
}

void *memset(void *destination, int value, size_t length)
{
	unsigned char *destinationBytes = (unsigned char *)destination;
	size_t index;

	for (index = 0; index < length; index++) {
		destinationBytes[index] = (unsigned char)value;
	}
	return destination;
}

/* The bootloader only uses the stack-based one-shot LZ4 API. */
void *malloc(size_t size)
{
	(void)size;
	return (void *)0;
}

void *calloc(size_t count, size_t size)
{
	(void)count;
	(void)size;
	return (void *)0;
}

void free(void *pointer)
{
	(void)pointer;
}

void _init(void)
{
}

void _exit(int status)
{
	(void)status;
	for (;;) {
		__asm__ volatile ("debug");
	}
}
