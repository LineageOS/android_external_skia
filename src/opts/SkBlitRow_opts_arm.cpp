/*
 * Copyright 2012 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBlitRow.h"
#include "SkColorPriv.h"
#include "SkDither.h"
#include "SkMathPriv.h"
#include "SkUtils.h"
#include "SkUtilsArm.h"

// Define USE_NEON_CODE to indicate that we need to build NEON routines
#define USE_NEON_CODE  (!SK_ARM_NEON_IS_NONE)

// Define USE_ARM_CODE to indicate that we need to build ARM routines
#define USE_ARM_CODE   (!SK_ARM_NEON_IS_ALWAYS)

#if USE_NEON_CODE
  #include "SkBlitRow_opts_arm_neon.h"
#endif

#if USE_ARM_CODE

static void S32A_D565_Opaque(uint16_t* SK_RESTRICT dst,
                             const SkPMColor* SK_RESTRICT src, int count,
                             U8CPU alpha, int /*x*/, int /*y*/) {
    SkASSERT(255 == alpha);

    asm volatile (
                  "1:                                   \n\t"
                  "ldr     r3, [%[src]], #4             \n\t"
                  "cmp     r3, #0xff000000              \n\t"
                  "blo     2f                           \n\t"
                  "and     r4, r3, #0x0000f8            \n\t"
                  "and     r5, r3, #0x00fc00            \n\t"
                  "and     r6, r3, #0xf80000            \n\t"
#ifdef SK_ARM_HAS_EDSP
                  "pld     [r1, #32]                    \n\t"
#endif
                  "lsl     r3, r4, #8                   \n\t"
                  "orr     r3, r3, r5, lsr #5           \n\t"
                  "orr     r3, r3, r6, lsr #19          \n\t"
                  "subs    %[count], %[count], #1       \n\t"
                  "strh    r3, [%[dst]], #2             \n\t"
                  "bne     1b                           \n\t"
                  "b       4f                           \n\t"
                  "2:                                   \n\t"
                  "lsrs    r7, r3, #24                  \n\t"
                  "beq     3f                           \n\t"
                  "ldrh    r4, [%[dst]]                 \n\t"
                  "rsb     r7, r7, #255                 \n\t"
                  "and     r6, r4, #0x001f              \n\t"
#if SK_ARM_ARCH <= 6
                  "lsl     r5, r4, #21                  \n\t"
                  "lsr     r5, r5, #26                  \n\t"
#else
                  "ubfx    r5, r4, #5, #6               \n\t"
#endif
#ifdef SK_ARM_HAS_EDSP
                  "pld     [r0, #16]                    \n\t"
#endif
                  "lsr     r4, r4, #11                  \n\t"
#ifdef SK_ARM_HAS_EDSP
                  "smulbb  r6, r6, r7                   \n\t"
                  "smulbb  r5, r5, r7                   \n\t"
                  "smulbb  r4, r4, r7                   \n\t"
#else
                  "mul     r6, r6, r7                   \n\t"
                  "mul     r5, r5, r7                   \n\t"
                  "mul     r4, r4, r7                   \n\t"
#endif
#if SK_ARM_ARCH >= 6
                  "uxtb    r7, r3, ROR #16              \n\t"
                  "uxtb    ip, r3, ROR #8               \n\t"
#else
                  "mov     ip, #0xff                    \n\t"
                  "and     r7, ip, r3, ROR #16          \n\t"
                  "and     ip, ip, r3, ROR #8           \n\t"
#endif
                  "and     r3, r3, #0xff                \n\t"
                  "add     r6, r6, #16                  \n\t"
                  "add     r5, r5, #32                  \n\t"
                  "add     r4, r4, #16                  \n\t"
                  "add     r6, r6, r6, lsr #5           \n\t"
                  "add     r5, r5, r5, lsr #6           \n\t"
                  "add     r4, r4, r4, lsr #5           \n\t"
                  "add     r6, r7, r6, lsr #5           \n\t"
                  "add     r5, ip, r5, lsr #6           \n\t"
                  "add     r4, r3, r4, lsr #5           \n\t"
                  "lsr     r6, r6, #3                   \n\t"
                  "and     r5, r5, #0xfc                \n\t"
                  "and     r4, r4, #0xf8                \n\t"
                  "orr     r6, r6, r5, lsl #3           \n\t"
                  "orr     r4, r6, r4, lsl #8           \n\t"
                  "strh    r4, [%[dst]], #2             \n\t"
#ifdef SK_ARM_HAS_EDSP
                  "pld     [r1, #32]                    \n\t"
#endif
                  "subs    %[count], %[count], #1       \n\t"
                  "bne     1b                           \n\t"
                  "b       4f                           \n\t"
                  "3:                                   \n\t"
                  "subs    %[count], %[count], #1       \n\t"
                  "add     %[dst], %[dst], #2           \n\t"
                  "bne     1b                           \n\t"
                  "4:                                   \n\t"
                  : [dst] "+r" (dst), [src] "+r" (src), [count] "+r" (count)
                  :
                  : "memory", "cc", "r3", "r4", "r5", "r6", "r7", "ip"
                  );
}

static void S32A_Opaque_BlitRow32_arm(SkPMColor* SK_RESTRICT dst,
                                  const SkPMColor* SK_RESTRICT src,
                                  int count, U8CPU alpha) {

    SkASSERT(255 == alpha);

    asm volatile (
                  "cmp    %[count], #0               \n\t" /* comparing count with 0 */
                  "beq    3f                         \n\t" /* if zero exit */

                  "mov    ip, #0xff                  \n\t" /* load the 0xff mask in ip */
                  "orr    ip, ip, ip, lsl #16        \n\t" /* convert it to 0xff00ff in ip */

                  "cmp    %[count], #2               \n\t" /* compare count with 2 */
                  "blt    2f                         \n\t" /* if less than 2 -> single loop */

                  /* Double Loop */
                  "1:                                \n\t" /* <double loop> */
                  "ldm    %[src]!, {r5,r6}           \n\t" /* load the src(s) at r5-r6 */
                  "ldm    %[dst], {r7,r8}            \n\t" /* loading dst(s) into r7-r8 */
                  "lsr    r4, r5, #24                \n\t" /* extracting the alpha from source and storing it to r4 */

                  /* ----------- */
                  "and    r9, ip, r7                 \n\t" /* r9 = br masked by ip */
                  "rsb    r4, r4, #256               \n\t" /* subtracting the alpha from 256 -> r4=scale */
                  "and    r10, ip, r7, lsr #8        \n\t" /* r10 = ag masked by ip */

                  "mul    r9, r9, r4                 \n\t" /* br = br * scale */
                  "mul    r10, r10, r4               \n\t" /* ag = ag * scale */
                  "and    r9, ip, r9, lsr #8         \n\t" /* lsr br by 8 and mask it */

                  "and    r10, r10, ip, lsl #8       \n\t" /* mask ag with reverse mask */
                  "lsr    r4, r6, #24                \n\t" /* extracting the alpha from source and storing it to r4 */
                  "orr    r7, r9, r10                \n\t" /* br | ag*/

                  "add    r7, r5, r7                 \n\t" /* dst = src + calc dest(r7) */
                  "rsb    r4, r4, #256               \n\t" /* subtracting the alpha from 255 -> r4=scale */

                  /* ----------- */
                  "and    r9, ip, r8                 \n\t" /* r9 = br masked by ip */

                  "and    r10, ip, r8, lsr #8        \n\t" /* r10 = ag masked by ip */
                  "mul    r9, r9, r4                 \n\t" /* br = br * scale */
                  "sub    %[count], %[count], #2     \n\t"
                  "mul    r10, r10, r4               \n\t" /* ag = ag * scale */

                  "and    r9, ip, r9, lsr #8         \n\t" /* lsr br by 8 and mask it */
                  "and    r10, r10, ip, lsl #8       \n\t" /* mask ag with reverse mask */
                  "cmp    %[count], #1               \n\t" /* comparing count with 1 */
                  "orr    r8, r9, r10                \n\t" /* br | ag */

                  "add    r8, r6, r8                 \n\t" /* dst = src + calc dest(r8) */

                  /* ----------------- */
                  "stm    %[dst]!, {r7,r8}           \n\t" /* *dst = r7, increment dst by two (each times 4) */
                  /* ----------------- */

                  "bgt    1b                         \n\t" /* if greater than 1 -> reloop */
                  "blt    3f                         \n\t" /* if less than 1 -> exit */

                  /* Single Loop */
                  "2:                                \n\t" /* <single loop> */
                  "ldr    r5, [%[src]], #4           \n\t" /* load the src pointer into r5 r5=src */
                  "ldr    r7, [%[dst]]               \n\t" /* loading dst into r7 */
                  "lsr    r4, r5, #24                \n\t" /* extracting the alpha from source and storing it to r4 */

                  /* ----------- */
                  "and    r9, ip, r7                 \n\t" /* r9 = br masked by ip */
                  "rsb    r4, r4, #256               \n\t" /* subtracting the alpha from 256 -> r4=scale */

                  "and    r10, ip, r7, lsr #8        \n\t" /* r10 = ag masked by ip */
                  "mul    r9, r9, r4                 \n\t" /* br = br * scale */
                  "mul    r10, r10, r4               \n\t" /* ag = ag * scale */
                  "and    r9, ip, r9, lsr #8         \n\t" /* lsr br by 8 and mask it */

                  "and    r10, r10, ip, lsl #8       \n\t" /* mask ag */
                  "orr    r7, r9, r10                \n\t" /* br | ag */

                  "add    r7, r5, r7                 \n\t" /* *dst = src + calc dest(r7) */

                  /* ----------------- */
                  "str    r7, [%[dst]], #4           \n\t" /* *dst = r7, increment dst by one (times 4) */
                  /* ----------------- */

                  "3:                                \n\t" /* <exit> */
                  : [dst] "+r" (dst), [src] "+r" (src), [count] "+r" (count)
                  :
                  : "cc", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "ip", "memory"
                  );
}

#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN) && defined(ENABLE_OPTIMIZED_S32A_BLITTERS)

/* This function was broken out to keep GCC from storing all registers on the stack
   even though they would not be used in the assembler code */
static __attribute__ ((noinline)) void S32A_D565_Opaque_Dither_Handle8(uint16_t * SK_RESTRICT dst,
                                                                       const SkPMColor* SK_RESTRICT src,
                                                                       int count, U8CPU alpha, int x, int y) {
    DITHER_565_SCAN(y);
    do {
        SkPMColor c = *src++;
        SkPMColorAssert(c);
        if (c) {
            unsigned a = SkGetPackedA32(c);

            // dither and alpha are just temporary variables to work-around
            // an ICE in debug.
            unsigned dither = DITHER_VALUE(x);
            unsigned alpha = SkAlpha255To256(a);
            int d = SkAlphaMul(dither, alpha);

            unsigned sr = SkGetPackedR32(c);
            unsigned sg = SkGetPackedG32(c);
            unsigned sb = SkGetPackedB32(c);
            sr = SkDITHER_R32_FOR_565(sr, d);
            sg = SkDITHER_G32_FOR_565(sg, d);
            sb = SkDITHER_B32_FOR_565(sb, d);

            uint32_t src_expanded = (sg << 24) | (sr << 13) | (sb << 2);
            uint32_t dst_expanded = SkExpand_rgb_16(*dst);
            dst_expanded = dst_expanded * (SkAlpha255To256(255 - a) >> 3);
            // now src and dst expanded are in g:11 r:10 x:1 b:10
            *dst = SkCompact_rgb_16((src_expanded + dst_expanded) >> 5);
        }
        dst += 1;
        DITHER_INC_X(x);
    } while (--count != 0);
}


static void S32A_D565_Opaque_Dither_neon(uint16_t * SK_RESTRICT dst,
                                         const SkPMColor* SK_RESTRICT src,
                                         int count, U8CPU alpha, int x, int y) {
    SkASSERT(255 == alpha);

    if (count >= 8) {
        asm volatile (
                    "pld            [%[src]]                        \n\t"   // Preload source
                    "pld            [%[dst]]                        \n\t"   // Preload destination pixels
                    "and            %[y], %[y], #0x03               \n\t"   // Mask y by 3
                    "vmov.i8        d31, #0x01                      \n\t"   // Set up alpha constant
                    "add            %[y], %[y], lsl #1              \n\t"   // and multiply with 12 to get the row offset
                    "and            %[x], %[x], #0x03               \n\t"   // Mask x by 3
                    "vmov.i16       q12, #256                       \n\t"   // Set up alpha constant
                    "add            %[y], %[matrix], %[y], lsl #2   \n\t"   //
                    "add            r7, %[x], %[y]                  \n\t"   //
                    "vld1.8         {d26}, [r7]                     \n\t"   // Load dither values
                    "add            %[x], %[count]                  \n\t"   //
                    "vmov.i16       q11, #0x3F                      \n\t"   // Set up green mask constant
                    "and            %[x], %[x], #0x03               \n\t"   // Mask x by 3
                    "vmovl.u8       q13, d26                        \n\t"   // Expand dither to 16-bit
                    "add            r7, %[x], %[y]                  \n\t"   //
                    "vmov.i16       q10, #0x1F                      \n\t"   // Set up blue mask constant
                    "vld1.8         {d28}, [r7]                     \n\t"   // Load iteration 2+ dither values
                    "ands           r7, %[count], #7                \n\t"   // Calculate first iteration increment
                    "moveq          r7, #8                          \n\t"   // Do full iteration?
                    "vmovl.u8       q14, d28                        \n\t"   // Expand dither to 16-bit
                    "vld4.8         {d0-d3}, [%[src]]               \n\t"   // Load eight source pixels
                    "vld1.16        {q3}, [%[dst]]                  \n\t"   // Load destination 565 pixels
                    "add            %[src], r7, lsl #2              \n\t"   // Increment source pointer
                    "add            %[dst], r7, lsl #1              \n\t"   // Increment destination buffer pointer
                    "subs           %[count], r7                    \n\t"   // Decrement loop counter
                    "sub            r7, %[dst], r7, lsl #1          \n\t"   // Save original destination pointer
                    "b              2f                              \n\t"
                    "1:                                             \n\t"
                    "vld4.8         {d0-d3}, [%[src]]!              \n\t"   // Load eight source pixels
                    "vld1.16        {q3}, [%[dst]]!                 \n\t"   // Load destination 565 pixels
                    "vst1.16        {q2}, [r7]                      \n\t"   // Write result to memory
                    "sub            r7, %[dst], #8*2                \n\t"   // Calculate next loop's destination pointer
                    "subs           %[count], #8                    \n\t"   // Decrement loop counter
                    "2:                                             \n\t"
                    "pld            [%[src]]                        \n\t"   // Preload destination pixels
                    "pld            [%[dst]]                        \n\t"   // Preload destination pixels
                    "vaddl.u8       q2, d3, d31                     \n\t"   // Add 1 to alpha to get 0-256
                    "vshr.u8        d16, d0, #5                     \n\t"   // Calculate source red subpixel
                    "vmul.u16       q2, q2, q13                     \n\t"   // Multiply alpha with dither value
                    "vsub.i8        d0, d16                         \n\t"   // red = (red - (red >> 5) + dither)
                    "vshrn.i16      d30, q2, #8                     \n\t"   // Shift and narrow result to 0-7
                    "vadd.i8        d0, d30                         \n\t"   //
                    "vshr.u8        d16, d2, #5                     \n\t"   // Calculate source blue subpixel
                    "vsub.i8        d2, d16                         \n\t"   // blue = (blue - (blue >> 5) + dither)
                    "vshr.u8        d16, d1, #6                     \n\t"   // Calculate source green subpixel
                    "vadd.i8        d2, d30                         \n\t"   //
                    "vsub.i8        d1, d16                         \n\t"   // green = (green - (green >> 6) + (dither >> 1))
                    "vshr.u8        d30, #1                         \n\t"   //
                    "vadd.i8        d1, d30                         \n\t"   //
                    "vsubw.u8       q2, q12, d3                     \n\t"   // Calculate inverse alpha 256-1
                    "vshr.u16       q8, q3, #5                      \n\t"   // Extract destination green pixel
                    "vshr.u16       q9, q3, #11                     \n\t"   // Extract destination red pixel
                    "vand           q8, q11                         \n\t"   // Shift green
                    "vand           q3, q10                         \n\t"   // Extract destination blue pixel
                    "vshr.u16       q2, #3                          \n\t"   // Shift alpha
                    "vshll.u8       q1, d2, #2                      \n\t"   // Calculate destination blue pixel
                    "vmla.i16       q1, q3, q2                      \n\t"   // ...and add to source pixel
                    "vshll.u8       q3, d1, #3                      \n\t"   // Calculate destination green pixel
                    "vmov.u8        q13, q14                        \n\t"   // Set dither matrix to iteration 2+ values
                    "vmla.i16       q3, q8, q2                      \n\t"   // ...and add to source pixel
                    "vshll.u8       q8, d0, #2                      \n\t"   // Calculate destination red pixel
                    "vmla.i16       q8, q9, q2                      \n\t"   // ...and add to source pixel
                    "vshr.u16       q1, #5                          \n\t"   // Pack blue pixel
                    "vand           q2, q1, q10                     \n\t"   //
                    "vshr.u16       q3, #5                          \n\t"   // Pack green pixel
                    "vsli.16        q2, q3, #5                      \n\t"   // ...and insert
                    "vshr.u16       q8, #5                          \n\t"   // Pack red pixel
                    "vsli.16        q2, q8, #11                     \n\t"   // ...and insert
                    "bne            1b                              \n\t"   // If inner loop counter != 0, loop
                    "vst1.16        {q2}, [r7]                      \n\t"   // Write result to memory
                    : [src] "+r" (src), [dst] "+r" (dst), [count] "+r" (count), [x] "+r" (x), [y] "+r" (y)
                    : [matrix] "r" (gDitherMatrix_Neon)
                    : "cc", "memory", "r7", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"
                    );
    }
    else {
        S32A_D565_Opaque_Dither_Handle8(dst, src, count, alpha, x, y);
    }
}

#define	S32A_D565_Opaque_Dither_PROC S32A_D565_Opaque_Dither_neon
#endif

#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN) && defined(ENABLE_OPTIMIZED_S32A_BLITTERS)

/* External function in file S32A_Blend_BlitRow32_neon.S */
extern "C" void S32A_Blend_BlitRow32_neon(SkPMColor* SK_RESTRICT dst,
                                          const SkPMColor* SK_RESTRICT src,
                                          int count, U8CPU alpha);

#define S32A_Blend_BlitRow32_PROC  S32A_Blend_BlitRow32_neon
#else

/*
 * ARM asm version of S32A_Blend_BlitRow32
 */
void S32A_Blend_BlitRow32_arm(SkPMColor* SK_RESTRICT dst,
                              const SkPMColor* SK_RESTRICT src,
                              int count, U8CPU alpha) {
    asm volatile (
                  "cmp    %[count], #0               \n\t" /* comparing count with 0 */
                  "beq    3f                         \n\t" /* if zero exit */

                  "mov    r12, #0xff                 \n\t" /* load the 0xff mask in r12 */
                  "orr    r12, r12, r12, lsl #16     \n\t" /* convert it to 0xff00ff in r12 */

                  /* src1,2_scale */
                  "add    %[alpha], %[alpha], #1     \n\t" /* loading %[alpha]=src_scale=alpha+1 */

                  "cmp    %[count], #2               \n\t" /* comparing count with 2 */
                  "blt    2f                         \n\t" /* if less than 2 -> single loop */

                  /* Double Loop */
                  "1:                                \n\t" /* <double loop> */
                  "ldm    %[src]!, {r5, r6}          \n\t" /* loading src pointers into r5 and r6 */
                  "ldm    %[dst], {r7, r8}           \n\t" /* loading dst pointers into r7 and r8 */

                  /* dst1_scale and dst2_scale*/
                  "lsr    r9, r5, #24                \n\t" /* src >> 24 */
                  "lsr    r10, r6, #24               \n\t" /* src >> 24 */
#ifdef SK_ARM_HAS_EDSP
                  "smulbb r9, r9, %[alpha]           \n\t" /* r9 = SkMulS16 r9 with src_scale */
                  "smulbb r10, r10, %[alpha]         \n\t" /* r10 = SkMulS16 r10 with src_scale */
#else
                  "mul    r9, r9, %[alpha]           \n\t" /* r9 = SkMulS16 r9 with src_scale */
                  "mul    r10, r10, %[alpha]         \n\t" /* r10 = SkMulS16 r10 with src_scale */
#endif
                  "lsr    r9, r9, #8                 \n\t" /* r9 >> 8 */
                  "lsr    r10, r10, #8               \n\t" /* r10 >> 8 */
                  "rsb    r9, r9, #256               \n\t" /* dst1_scale = r9 = 255 - r9 + 1 */
                  "rsb    r10, r10, #256             \n\t" /* dst2_scale = r10 = 255 - r10 + 1 */

                  /* ---------------------- */

                  /* src1, src1_scale */
                  "and    r11, r12, r5, lsr #8       \n\t" /* ag = r11 = r5 masked by r12 lsr by #8 */
                  "and    r4, r12, r5                \n\t" /* rb = r4 = r5 masked by r12 */
                  "mul    r11, r11, %[alpha]         \n\t" /* ag = r11 times src_scale */
                  "mul    r4, r4, %[alpha]           \n\t" /* rb = r4 times src_scale */
                  "and    r11, r11, r12, lsl #8      \n\t" /* ag masked by reverse mask (r12) */
                  "and    r4, r12, r4, lsr #8        \n\t" /* rb masked by mask (r12) */
                  "orr    r5, r11, r4                \n\t" /* r5 = (src1, src_scale) */

                  /* dst1, dst1_scale */
                  "and    r11, r12, r7, lsr #8       \n\t" /* ag = r11 = r7 masked by r12 lsr by #8 */
                  "and    r4, r12, r7                \n\t" /* rb = r4 = r7 masked by r12 */
                  "mul    r11, r11, r9               \n\t" /* ag = r11 times dst_scale (r9) */
                  "mul    r4, r4, r9                 \n\t" /* rb = r4 times dst_scale (r9) */
                  "and    r11, r11, r12, lsl #8      \n\t" /* ag masked by reverse mask (r12) */
                  "and    r4, r12, r4, lsr #8        \n\t" /* rb masked by mask (r12) */
                  "orr    r9, r11, r4                \n\t" /* r9 = (dst1, dst_scale) */

                  /* ---------------------- */
                  "add    r9, r5, r9                 \n\t" /* *dst = src plus dst both scaled */
                  /* ---------------------- */

                  /* ====================== */

                  /* src2, src2_scale */
                  "and    r11, r12, r6, lsr #8       \n\t" /* ag = r11 = r6 masked by r12 lsr by #8 */
                  "and    r4, r12, r6                \n\t" /* rb = r4 = r6 masked by r12 */
                  "mul    r11, r11, %[alpha]         \n\t" /* ag = r11 times src_scale */
                  "mul    r4, r4, %[alpha]           \n\t" /* rb = r4 times src_scale */
                  "and    r11, r11, r12, lsl #8      \n\t" /* ag masked by reverse mask (r12) */
                  "and    r4, r12, r4, lsr #8        \n\t" /* rb masked by mask (r12) */
                  "orr    r6, r11, r4                \n\t" /* r6 = (src2, src_scale) */

                  /* dst2, dst2_scale */
                  "and    r11, r12, r8, lsr #8       \n\t" /* ag = r11 = r8 masked by r12 lsr by #8 */
                  "and    r4, r12, r8                \n\t" /* rb = r4 = r8 masked by r12 */
                  "mul    r11, r11, r10              \n\t" /* ag = r11 times dst_scale (r10) */
                  "mul    r4, r4, r10                \n\t" /* rb = r4 times dst_scale (r6) */
                  "and    r11, r11, r12, lsl #8      \n\t" /* ag masked by reverse mask (r12) */
                  "and    r4, r12, r4, lsr #8        \n\t" /* rb masked by mask (r12) */
                  "orr    r10, r11, r4               \n\t" /* r10 = (dst2, dst_scale) */

                  "sub    %[count], %[count], #2     \n\t" /* decrease count by 2 */
                  /* ---------------------- */
                  "add    r10, r6, r10               \n\t" /* *dst = src plus dst both scaled */
                  /* ---------------------- */
                  "cmp    %[count], #1               \n\t" /* compare count with 1 */
                  /* ----------------- */
                  "stm    %[dst]!, {r9, r10}         \n\t" /* copy r9 and r10 to r7 and r8 respectively */
                  /* ----------------- */

                  "bgt    1b                         \n\t" /* if %[count] greater than 1 reloop */
                  "blt    3f                         \n\t" /* if %[count] less than 1 exit */
                                                           /* else get into the single loop */
                  /* Single Loop */
                  "2:                                \n\t" /* <single loop> */
                  "ldr    r5, [%[src]], #4           \n\t" /* loading src pointer into r5: r5=src */
                  "ldr    r7, [%[dst]]               \n\t" /* loading dst pointer into r7: r7=dst */

                  "lsr    r6, r5, #24                \n\t" /* src >> 24 */
                  "and    r8, r12, r5, lsr #8        \n\t" /* ag = r8 = r5 masked by r12 lsr by #8 */
#ifdef SK_ARM_HAS_EDSP
                  "smulbb r6, r6, %[alpha]           \n\t" /* r6 = SkMulS16 with src_scale */
#else
                  "mul    r6, r6, %[alpha]           \n\t" /* r6 = SkMulS16 with src_scale */
#endif
                  "and    r9, r12, r5                \n\t" /* rb = r9 = r5 masked by r12 */
                  "lsr    r6, r6, #8                 \n\t" /* r6 >> 8 */
                  "mul    r8, r8, %[alpha]           \n\t" /* ag = r8 times scale */
                  "rsb    r6, r6, #256               \n\t" /* r6 = 255 - r6 + 1 */

                  /* src, src_scale */
                  "mul    r9, r9, %[alpha]           \n\t" /* rb = r9 times scale */
                  "and    r8, r8, r12, lsl #8        \n\t" /* ag masked by reverse mask (r12) */
                  "and    r9, r12, r9, lsr #8        \n\t" /* rb masked by mask (r12) */
                  "orr    r10, r8, r9                \n\t" /* r10 = (scr, src_scale) */

                  /* dst, dst_scale */
                  "and    r8, r12, r7, lsr #8        \n\t" /* ag = r8 = r7 masked by r12 lsr by #8 */
                  "and    r9, r12, r7                \n\t" /* rb = r9 = r7 masked by r12 */
                  "mul    r8, r8, r6                 \n\t" /* ag = r8 times scale (r6) */
                  "mul    r9, r9, r6                 \n\t" /* rb = r9 times scale (r6) */
                  "and    r8, r8, r12, lsl #8        \n\t" /* ag masked by reverse mask (r12) */
                  "and    r9, r12, r9, lsr #8        \n\t" /* rb masked by mask (r12) */
                  "orr    r7, r8, r9                 \n\t" /* r7 = (dst, dst_scale) */

                  "add    r10, r7, r10               \n\t" /* *dst = src plus dst both scaled */

                  /* ----------------- */
                  "str    r10, [%[dst]], #4          \n\t" /* *dst = r10, postincrement dst by one (times 4) */
                  /* ----------------- */

                  "3:                                \n\t" /* <exit> */
                  : [dst] "+r" (dst), [src] "+r" (src), [count] "+r" (count), [alpha] "+r" (alpha)
                  :
                  : "cc", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "memory"
                  );

}
#endif

///////////////////////////////////////////////////////////////////////////////

static const SkBlitRow::Proc sk_blitrow_platform_565_procs_arm[] = {
    // no dither
    // NOTE: For the functions below, we don't have a special version
    //       that assumes that each source pixel is opaque. But our S32A is
    //       still faster than the default, so use it.
    S32A_D565_Opaque,   // S32_D565_Opaque
    NULL,               // S32_D565_Blend
    S32A_D565_Opaque,   // S32A_D565_Opaque
    NULL,               // S32A_D565_Blend

    // dither
    NULL,   // S32_D565_Opaque_Dither
    NULL,   // S32_D565_Blend_Dither
    NULL,   // S32A_D565_Opaque_Dither
    NULL,   // S32A_D565_Blend_Dither
};

#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN) && defined(ENABLE_OPTIMIZED_S32A_BLITTERS)

/* External function in file S32A_Opaque_BlitRow32_neon.S */
			extern "C" void S32A_Opaque_BlitRow32_neon(SkPMColor* SK_RESTRICT dst,
                                           const SkPMColor* SK_RESTRICT src,
                                           int count, U8CPU alpha);

#define S32A_Opaque_BlitRow32_PROC  S32A_Opaque_BlitRow32_neon
#endif

static const SkBlitRow::Proc32 sk_blitrow_platform_32_procs_arm[] = {
    NULL,   // S32_Opaque,
    NULL,   // S32_Blend,
    S32A_Opaque_BlitRow32_arm,   // S32A_Opaque,
    S32A_Blend_BlitRow32_arm     // S32A_Blend
};

#endif // USE_ARM_CODE

extern SkBlitRow::Proc32 skia_androidopt_PlatformProcs32(unsigned flags) __attribute__((weak));
extern SkBlitRow::Proc skia_androidopt_PlatformProcs565(unsigned flags) __attribute__((weak));

SkBlitRow::Proc SkBlitRow::PlatformProcs565(unsigned flags) {
    if (skia_androidopt_PlatformProcs565 && skia_androidopt_PlatformProcs565(flags) ) {
        return  skia_androidopt_PlatformProcs565(flags);
    } else {
        return SK_ARM_NEON_WRAP(sk_blitrow_platform_565_procs_arm)[flags];
    }
}


SkBlitRow::Proc32 SkBlitRow::PlatformProcs32(unsigned flags) {
    if (skia_androidopt_PlatformProcs32 && skia_androidopt_PlatformProcs32(flags) ) {
        return  skia_androidopt_PlatformProcs32(flags);
    } else {
        return SK_ARM_NEON_WRAP(sk_blitrow_platform_32_procs_arm)[flags];
    }
}

///////////////////////////////////////////////////////////////////////////////
#define Color32_arm  NULL
SkBlitRow::ColorProc SkBlitRow::PlatformColorProc() {
    return SK_ARM_NEON_WRAP(Color32_arm);
}
