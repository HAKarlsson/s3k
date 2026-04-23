#pragma once

/**
 * @brief Asserts a condition at runtime in debug builds.
 *
 * In debug builds (when NDEBUG is not defined), if the condition is false,
 * execution is halted via an `ebreak` instruction (RISC-V software breakpoint).
 * In release builds, the expression is evaluated but no check is performed.
 *
 * @param x The condition to assert.
 */
#ifdef NDEBUG
#define KASSERT(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#else
#define KASSERT(x) do { if (!(x)) __asm__ volatile("ebreak"); } while (0)
#endif

/**
 * @brief Calculates the number of elements in an array.
 *
 * @param a The array whose size is to be calculated.
 * @return The number of elements in the array.
 */
#define ARRAY_SIZE(a) (sizeof(a) / (sizeof((a)[0])))

/**
 * @brief Hints to the compiler that a condition is unlikely to be true.
 *
 * This macro uses GCC's `__builtin_expect` to optimize branch prediction.
 *
 * @param x The condition to evaluate.
 * @return The value of the condition.
 */
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

/**
 * @brief Hints to the compiler that a condition is likely to be true.
 *
 * This macro uses GCC's `__builtin_expect` to optimize branch prediction.
 *
 * @param x The condition to evaluate.
 * @return The value of the condition.
 */
#define LIKELY(x) __builtin_expect(!!(x), 1)

/**
 * @brief Aligns a value to the nearest higher multiple of a given alignment.
 *
 * @param value The value to align.
 * @param alignment The alignment boundary (must be a power of 2).
 * @return The aligned value.
 */
#define ALIGN_UP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))

/**
 * @brief Aligns a value to the nearest lower multiple of a given alignment.
 *
 * @param value The value to align.
 * @param alignment The alignment boundary (must be a power of 2).
 * @return The aligned value.
 */
#define ALIGN_DOWN(value, alignment) ((value) & ~((alignment) - 1))

/**
 * @brief Converts a value from bytes to kilobytes.
 *
 * @param bytes The value in bytes.
 * @return The value in kilobytes.
 */
#define BYTES_TO_KB(bytes) ((bytes) / 1024)

/**
 * @brief Converts a value from bytes to megabytes.
 *
 * @param bytes The value in bytes.
 * @return The value in megabytes.
 */
#define BYTES_TO_MB(bytes) ((bytes) / (1024 * 1024))
