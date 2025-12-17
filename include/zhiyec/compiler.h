#ifndef _ZHIYEC_COMPILER_H
#define _ZHIYEC_COMPILER_H

#if defined(__GNUC__)
#define always_inline __attribute__((always_inline))
#define section(x) __attribute__((section(x)))
#define typeof __typeof__
#define used __attribute__((used))
#elif defined(__ARMCC_VERSION)
#define always_inline __forceinline
#define section(x) __attribute__((section(x)))
#define typeof __typeof
#define used __attribute__((used))
#else
#define always_inline
#define section(x)
#define typeof
#define used
#endif

#if defined(__ARMCC_VERSION)
#define DSB() __dsb(0U)
#define ISB() __isb(0U)
#define DMB() __dmb(0U)
#else
#define DSB()
#define ISB()
#define DMB()
#endif

#endif /* _ZHIYEC_COMPILER_H */
