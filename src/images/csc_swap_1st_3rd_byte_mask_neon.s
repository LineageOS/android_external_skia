/*
 *
 * Copyright 2010 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file    csc_swap_1st_3rd_byte_neon.s
 * @brief   swap 1st and 3rd byte in word
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2011.11.11 : Create
 */

    .arch armv7-a
    .text
    .global csc_swap_1st_3rd_byte_mask_neon
    .type   csc_swap_1st_3rd_byte_mask_neon, %function
csc_swap_1st_3rd_byte_mask_neon:
    .fnstart

    @r0     dest
    @r1     src
    @r2     size(word)
    @r3     value(byte)
    @r4
    @r5     i
    @r6     dest_addr
    @r7     src_addr
    @r8
    @r9
    @r10    temp1
    @r11    temp2
    @r12    temp3
    @r14    temp4

    stmfd       sp!, {r4-r12,r14}           @ backup registers

    mov         r6, r0
    mov         r7, r1
    mov         r5, r2
    cmp         r3, #0x0
    beq         MASK_00

MASK_FF:
    cmp         r5, #64
    blt         MASK_FF_LESS_THAN_64
MASK_FF_LOOP_64:
    pld         [r7, #32]
    vld4.8      {d0, d2, d4, d6}, [r7]!
    vld4.8      {d1, d3, d5, d7}, [r7]!
    pld         [r7, #32]
    vld4.8      {d8, d10, d12, d14}, [r7]!
    vld4.8      {d9, d11, d13, d15}, [r7]!
    pld         [r7, #32]
    vld4.8      {d16, d18, d20, d22}, [r7]!
    vld4.8      {d17, d19, d21, d23}, [r7]!
    pld         [r7, #32]
    vld4.8      {d24, d26, d28, d30}, [r7]!
    vld4.8      {d25, d27, d29, d31}, [r7]!
    vswp.8      q0, q2
    vswp.8      q4, q6
    vswp.8      q8, q10
    vswp.8      q12, q14
    vmov.i64    q3, #0xFFFFFFFFFFFFFFFF
    vmov.i64    q7, #0xFFFFFFFFFFFFFFFF
    vmov.i64    q11, #0xFFFFFFFFFFFFFFFF
    vmov.i64    q15, #0xFFFFFFFFFFFFFFFF
    vst4.8      {d0, d2, d4, d6}, [r6]!
    vst4.8      {d1, d3, d5, d7}, [r6]!
    vst4.8      {d8, d10, d12, d14}, [r6]!
    vst4.8      {d9, d11, d13, d15}, [r6]!
    vst4.8      {d16, d18, d20, d22}, [r6]!
    vst4.8      {d17, d19, d21, d23}, [r6]!
    vst4.8      {d24, d26, d28, d30}, [r6]!
    vst4.8      {d25, d27, d29, d31}, [r6]!
    sub         r5, #64
    cmp         r5, #64
    bge         MASK_FF_LOOP_64

MASK_FF_LESS_THAN_64:
    cmp         r5, #32
    blt         MASK_FF_LESS_THAN_32
    pld         [r7, #32]
    vld4.8      {d0, d2, d4, d6}, [r7]!
    vld4.8      {d1, d3, d5, d7}, [r7]!
    pld         [r7, #32]
    vld4.8      {d8, d10, d12, d14}, [r7]!
    vld4.8      {d9, d11, d13, d15}, [r7]!
    vswp.8      q0, q2
    vswp.8      q4, q6
    vmov.i64    q3, #0xFFFFFFFFFFFFFFFF
    vmov.i64    q7, #0xFFFFFFFFFFFFFFFF
    vst4.8      {d0, d2, d4, d6}, [r6]!
    vst4.8      {d1, d3, d5, d7}, [r6]!
    vst4.8      {d8, d10, d12, d14}, [r6]!
    vst4.8      {d9, d11, d13, d15}, [r6]!
    sub         r5, #32

MASK_FF_LESS_THAN_32:
    cmp         r5, #16
    blt         MASK_FF_LESS_THAN_16
    pld         [r7, #32]
    vld4.8      {d0, d2, d4, d6}, [r7]!
    vld4.8      {d1, d3, d5, d7}, [r7]!
    vswp.8      q0, q2
    vmov.i64    q3, #0xFFFFFFFFFFFFFFFF
    vst4.8      {d0, d2, d4, d6}, [r6]!
    vst4.8      {d1, d3, d5, d7}, [r6]!
    sub         r5, #16

MASK_FF_LESS_THAN_16:
    cmp         r5, #8
    blt         MASK_FF_LESS_THAN_8
    vld4.8      {d0, d2, d4, d6}, [r7]!
    vswp.8      d0, d4
    vmov.i32    d6, #0xFFFFFFFF
    vst4.8      {d0, d2, d4, d6}, [r6]!
    sub         r5, #8

MASK_FF_LESS_THAN_8:
    cmp         r5, #0
    beq         RESTORE_REG
    ldrb        r10, [r7], #1
    ldrb        r11, [r7], #1
    ldrb        r12, [r7], #1
    ldrb        r14, [r7], #1
    mov         r14, #0xFF
    strb        r12, [r6], #1
    strb        r11, [r6], #1
    strb        r10, [r6], #1
    strb        r14, [r6], #1

    subs        r5, #1
    bgt         MASK_FF_LESS_THAN_8
    b           RESTORE_REG

MASK_00:
    cmp         r5, #64
    blt         MASK_00_LESS_THAN_64
MASK_00_LOOP_64:
    pld         [r7, #32]
    vld4.8      {d0, d2, d4, d6}, [r7]!
    vld4.8      {d1, d3, d5, d7}, [r7]!
    pld         [r7, #32]
    vld4.8      {d8, d10, d12, d14}, [r7]!
    vld4.8      {d9, d11, d13, d15}, [r7]!
    pld         [r7, #32]
    vld4.8      {d16, d18, d20, d22}, [r7]!
    vld4.8      {d17, d19, d21, d23}, [r7]!
    pld         [r7, #32]
    vld4.8      {d24, d26, d28, d30}, [r7]!
    vld4.8      {d25, d27, d29, d31}, [r7]!
    vswp.8      q0, q2
    vswp.8      q4, q6
    vswp.8      q8, q10
    vswp.8      q12, q14
    vmov.i64    q3, #0x0000000000000000
    vmov.i64    q7, #0x0000000000000000
    vmov.i64    q11, #0x0000000000000000
    vmov.i64    q15, #0x0000000000000000
    vst4.8      {d0, d2, d4, d6}, [r6]!
    vst4.8      {d1, d3, d5, d7}, [r6]!
    vst4.8      {d8, d10, d12, d14}, [r6]!
    vst4.8      {d9, d11, d13, d15}, [r6]!
    vst4.8      {d16, d18, d20, d22}, [r6]!
    vst4.8      {d17, d19, d21, d23}, [r6]!
    vst4.8      {d24, d26, d28, d30}, [r6]!
    vst4.8      {d25, d27, d29, d31}, [r6]!
    sub         r5, #64
    cmp         r5, #64
    bge         MASK_00_LOOP_64

MASK_00_LESS_THAN_64:
    cmp         r5, #32
    blt         MASK_00_LESS_THAN_32
    pld         [r7, #32]
    vld4.8      {d0, d2, d4, d6}, [r7]!
    vld4.8      {d1, d3, d5, d7}, [r7]!
    pld         [r7, #32]
    vld4.8      {d8, d10, d12, d14}, [r7]!
    vld4.8      {d9, d11, d13, d15}, [r7]!
    vswp.8      q0, q2
    vswp.8      q4, q6
    vmov.i64    q3, #0x0000000000000000
    vmov.i64    q7, #0x0000000000000000
    vst4.8      {d0, d2, d4, d6}, [r6]!
    vst4.8      {d1, d3, d5, d7}, [r6]!
    vst4.8      {d8, d10, d12, d14}, [r6]!
    vst4.8      {d9, d11, d13, d15}, [r6]!
    sub         r5, #32

MASK_00_LESS_THAN_32:
    cmp         r5, #16
    blt         MASK_00_LESS_THAN_16
    pld         [r7, #32]
    vld4.8      {d0, d2, d4, d6}, [r7]!
    vld4.8      {d1, d3, d5, d7}, [r7]!
    vswp.8      q0, q2
    vmov.i64    q3, #0x0000000000000000
    vst4.8      {d0, d2, d4, d6}, [r6]!
    vst4.8      {d1, d3, d5, d7}, [r6]!
    sub         r5, #16

MASK_00_LESS_THAN_16:
    cmp         r5, #8
    blt         MASK_00_LESS_THAN_8
    vld4.8      {d0, d2, d4, d6}, [r7]!
    vswp.8      d0, d4
    vmov.i32    d6, #0x00000000
    vst4.8      {d0, d2, d4, d6}, [r6]!
    sub         r5, #8

MASK_00_LESS_THAN_8:
    cmp         r5, #0
    beq         RESTORE_REG
    ldrb        r10, [r7], #1
    ldrb        r11, [r7], #1
    ldrb        r12, [r7], #1
    ldrb        r14, [r7], #1
    mov         r14, #0x00
    strb        r12, [r6], #1
    strb        r11, [r6], #1
    strb        r10, [r6], #1
    strb        r14, [r6], #1

    subs        r5, #1
    bgt         MASK_00_LESS_THAN_8

RESTORE_REG:
    ldmfd       sp!, {r4-r12,r15}       @ restore registers

    .fnend
