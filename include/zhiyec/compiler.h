#ifndef _ZHIYEC_COMPILER_H
#define _ZHIYEC_COMPILER_H

#define FORCE_INLINE __forceinline

#define DSB() __dsb(0U)
#define ISB() __isb(0U)
#define DMB() __dmb(0U)

#endif /* _ZHIYEC_COMPILER_H */
