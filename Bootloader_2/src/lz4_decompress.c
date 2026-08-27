/*
 * Minimal heap-free LZ4 block decoder for the TC1791 BSL.
 *
 * This implements the LZ4_decompress_safe API used by MAIN.c without pulling
 * the allocation-backed streaming functions from the full upstream lz4.c into
 * the 32 KiB PSPR image.
 */

#include "lz4.h"

static int LZ4_readExtendedLength(const unsigned char **cursor,
		const unsigned char *end, unsigned int initial,
		unsigned int *result) {
	unsigned int length = initial;
	unsigned int extension;
	do {
		if (*cursor >= end) {
			return 0;
		}
		extension = **cursor;
		(*cursor)++;
		/* The BSL caps compressed input at 4128 bytes, so this cannot wrap. */
		length += extension;
	} while (extension == 255U);
	*result = length;
	return 1;
}

int LZ4_decompress_safe(const char *source, char *destination,
		int compressedSize, int destinationCapacity) {
	const unsigned char *input;
	const unsigned char *inputEnd;
	unsigned char *output;
	unsigned char *outputEnd;

	if (source == 0 || destination == 0 || compressedSize <= 0
			|| destinationCapacity < 0) {
		return -1;
	}
	input = (const unsigned char*) source;
	inputEnd = input + compressedSize;
	output = (unsigned char*) destination;
	outputEnd = output + destinationCapacity;

	while (input < inputEnd) {
		unsigned int token = *input++;
		unsigned int literalLength = token >> 4;
		unsigned int matchLength;
		unsigned int offset;
		unsigned char *match;

		if (literalLength == 15U
				&& !LZ4_readExtendedLength(&input, inputEnd, literalLength,
						&literalLength)) {
			return -1;
		}
		if (literalLength > (unsigned int) (inputEnd - input)
				|| literalLength > (unsigned int) (outputEnd - output)) {
			return -1;
		}
		while (literalLength-- > 0U) {
			*output++ = *input++;
		}

		/* A valid LZ4 block ends with a final literal-only sequence. */
		if (input == inputEnd) {
			return (int) (output - (unsigned char*) destination);
		}
		if ((inputEnd - input) < 2) {
			return -1;
		}
		offset = input[0] | ((unsigned int) input[1] << 8);
		input += 2;
		if (offset == 0U
				|| offset > (unsigned int) (output - (unsigned char*) destination)) {
			return -1;
		}

		matchLength = token & 0x0FU;
		if (matchLength == 15U
				&& !LZ4_readExtendedLength(&input, inputEnd, matchLength,
						&matchLength)) {
			return -1;
		}
		matchLength += 4U;
		if (matchLength > (unsigned int) (outputEnd - output)) {
			return -1;
		}
		match = output - offset;
		/* Byte-wise copying intentionally supports overlapping LZ4 matches. */
		while (matchLength-- > 0U) {
			*output++ = *match++;
		}
	}

	return (int) (output - (unsigned char*) destination);
}
