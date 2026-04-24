#pragma once

#include "types.h"

/// Maximum value of a PMP-encoded address. The physical address space is 56-bit,
/// and pmpaddr stores PA[55:2], so valid pmpaddr values are in [0, 2^54 - 1].
#define PMP_ADDR_MAX ((1ULL << 54) - 1)

/**
 * @brief Decode the base address from a PMP NAPOT-encoded address.
 *
 * @param addr The encoded NAPOT address.
 * @return The decoded base address.
 */
static inline word_t pmp_napot_decode_base(word_t addr)
{
	return ((addr + 1) & addr) << 2; // Extract the base address.
}

/**
 * @brief Decode the size from a PMP NAPOT-encoded address.
 *
 * @param addr The encoded NAPOT address.
 * @return The decoded size.
 */
static inline word_t pmp_napot_decode_size(word_t addr)
{
	return (((addr + 1) ^ addr) + 1) << 2; // Calculate the size.
}

/**
 * @brief Encode a base address and size into a PMP NAPOT address.
 *
 * @param base The base address to encode.
 * @param size The size (power of 2 >= 8) to encode.
 * @return The encoded NAPOT address.
 */
static inline word_t pmp_napot_encode(word_t base, word_t size)
{
	return (base | (size / 2 - 1)) >> 2; // Combine base and size into NAPOT format.
}
