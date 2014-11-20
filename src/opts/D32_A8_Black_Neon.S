// Copyright (c) 2013 The Linux Foundation. All rights reserved.
// Copyright (c) 2011 Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

.text

.global D32_A8_Black_Neon
.func D32_A8_Black_Neon


.macro  Blit_Single_Pixel_ARM
    ldr         r6, [r0]                //r6 = *device
    ldrb        r11, [r2], #1           //r11 = aa = *mask++;
    and         r8, r6, r10             //rb = (dst & mask)
    and         r9, r10, r6, lsr #8     //ag = (dst>>8) & mask
    rsb         r6, r11, #256           //r6 = scale = (255-alpha)+1
    lsl         r11, r11, #24           //r11 = (aa << SK_A32_SHIFT)
    mul         r12, r8, r6             //RB = rb * scale
    mul         r6, r9, r6              //AG = ag * scale
    and         r12, r10, r12, lsr #8   //r12 = (RB>>8) & mask
    and         r6, r6, r10, lsl #8     //r6 = AG & ~mask
    orr         r6, r6, r12             //(rb & mask) | (ag & ~mask);
    add         r12, r11, r6            //r12 = r11 + r6
    str         r12, [r0], #4           //*device = r12; device += 1;
.endm

.macro  Blit_Two_Pixels_ARM
    Blit_Single_Pixel_ARM
    Blit_Single_Pixel_ARM
.endm

.macro  Blit_Three_Pixels_ARM
    Blit_Two_Pixels_ARM
    Blit_Single_Pixel_ARM
.endm

.macro  Blit_Four_Pixels_NEON
    vld1.64     {d0, d1}, [r0]         //d0,d1 = *device r(0,1) g(0,1) b(0,1) A(0,1); device+=1;
    vld1.32     {d4[0]}, [r2]!         //unsigned aa = *mask++;
    vmovl.u8    q8,d0                  //Q8 = vmovl.u8 d0
    vmovl.u8    q9,d1                  //Q9 = vmovl.u8 d1
    vsubw.u8    q3,q14,d4              //Q3.16 = SkAlpha255To256(255 - aa)
    vdup.16     d2, d6[0]
    vdup.16     d3, d6[1]
    vdup.16     d20, d6[2]
    vdup.16     d21, d6[3]
    vmul.i16    q8,q8,q1               //Q8 = Q8 * Q4
    vmul.i16    q9,q9,q10              //Q7 = Q7 * Q4
    vmovl.u8    q1, d4                 //aa << 24
    vshrn.i16   d0,q8,#8               //d4 = Q8.16 shrn 8
    vmovl.u8    q10, d2                ////aa << 24
    vshrn.i16   d1,q9,#8               //d5 = Q7.16 shrn 8
    vrev32.8    q10, q10               ////aa << 24
    vadd.i8     q0,q0,q10              //d0 = d5+d0
    vst1.64     {d0, d1}, [r0]!        //*device = <result>
.endm

.macro  Blit_Eight_Pixels_NEON
    vld4.8           {d0, d1, d2, d3}, [r0] //d0,d1,d2,d3 = *device rgb(0,1,2,3) A(0,1,2,3); device+=1;
    vld1.8           {d4}, [r2]!            //unsigned aa = *mask++;
    vmovl.u8         q6,d0                  //Q6 = vmovl.u8 d0
    vmovl.u8         q7,d1                  //Q7 = vmovl.u8 d1
    vsubw.u8         q4,q14,d4              //Q4.16 = SkAlpha255To256(255 - aa)
    vmovl.u8         q8,d2                  //Q8 = vmovl.u8 d2
    vmul.i16         q6,q6,q4               //Q6 = Q6 * Q4
    vmovl.u8         q9,d3                  //Q9 = vmovl.u8 d3
    vmul.i16         q7,q7,q4               //Q7 = Q7 * Q4
    vshrn.i16        d0,q6,#8               //d4 = Q6.16 shrn 8
    vmul.i16         q9,q9,q4               //Q9 = Q9 * Q4
    vshrn.i16        d1,q7,#8               //d5 = Q7.16 shrn 8
    vmul.i16         q8,q8,q4               //Q8 = Q8 * Q4
    vshrn.i16        d3,q9,#8               //d7 = Q9.16 shrn 8
    vshrn.i16        d2,q8,#8               //d6 = Q8.16 shrn 8
    vadd.i8          d3,d3,d4               //d0 = d4+d0
    vst4.8           {d0, d1, d2, d3}, [r0]!//*device = <result>
.endm

.macro  Prefetch_Next_Data stride
    pld             [r0, r1]
    pld             [r2, r3]
    subs            r5, r5, #1              //Set the condition code flags for 'while'
.endm

.macro  Process_Next_Row_If_Needed loopStart
    add         r0,r0,r1                    //device = (uint32_t*)((char*)device + dstRB);
    add         r2,r2,r3                    //mask += maskRB;
    bne         \loopStart                  // } while (--height != 0);
    b           .LBlit_0_Pixel_Per_Row
.endm


D32_A8_Black_Neon:
    pld         [r0]                        //Preload 'device'
    pld         [r2]                        //Preload 'maskPtr'
    pld         [r2, #128]                  //Preload 'maskPtr' + 128
    push        {r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}

    mov         r10, #0xFF

    add         r8,sp,#40
    vpush       {q4-q7}
                                            //r0 - device (dst);
                                            //r1 - dstRB
                                            //r2 - maskPtr
                                            //r3 - maskRB

    ldr         r4,[r8, #4]                 //width
    ldr         r5,[r8, #8]                 //height

    sub         r3, r3, r4                  //maskRB -= width;
    mov         r6, r4, asl #2              //(width << 2)
    cmp         r4,#16
    sub         r1, r1, r6                  //dstRB -= (width << 2);

    orr         r10, r10, r10, lsl #16      //mask = r10 = 0x00FF00FF

    vmov.i16    q14,#0x100                  //Q14.16 = 256

    bgt         .Lgreater_than_16

    adr         r11, .LBrTable
    add         r11, r11, r4, lsl #2

    bx          r11
.LBrTable:
    b .LBlit_0_Pixel_Per_Row
    b .LBlit_1_Pixel_Per_Row
    b .LBlit_2_Pixels_Per_Row
    b .LBlit_3_Pixels_Per_Row
    b .LBlit_4_Pixels_Per_Row
    b .LBlit_5_Pixels_Per_Row
    b .LBlit_6_Pixels_Per_Row
    b .LBlit_7_Pixels_Per_Row
    b .LBlit_8_Pixels_Per_Row
    b .LBlit_9_Pixels_Per_Row
    b .LBlit_10_Pixels_Per_Row
    b .LBlit_11_Pixels_Per_Row
    b .LBlit_12_Pixels_Per_Row
    b .LBlit_13_Pixels_Per_Row
    b .LBlit_14_Pixels_Per_Row
    b .LBlit_15_Pixels_Per_Row
    b .LBlit_16_Pixels_Per_Row

.LBlit_1_Pixel_Per_Row:
    Prefetch_Next_Data #128
    Blit_Single_Pixel_ARM
    Process_Next_Row_If_Needed .LBlit_1_Pixel_Per_Row

.LBlit_2_Pixels_Per_Row:
    Prefetch_Next_Data #128
    Blit_Two_Pixels_ARM
    Process_Next_Row_If_Needed .LBlit_2_Pixels_Per_Row

.LBlit_3_Pixels_Per_Row:
    Prefetch_Next_Data #128
    Blit_Three_Pixels_ARM
    Process_Next_Row_If_Needed .LBlit_3_Pixels_Per_Row

.LBlit_4_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Four_Pixels_NEON
    Process_Next_Row_If_Needed .LBlit_4_Pixels_Per_Row

.LBlit_5_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Four_Pixels_NEON
    Blit_Single_Pixel_ARM
    Process_Next_Row_If_Needed .LBlit_5_Pixels_Per_Row

.LBlit_6_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Four_Pixels_NEON
    Blit_Two_Pixels_ARM
    Process_Next_Row_If_Needed .LBlit_6_Pixels_Per_Row

.LBlit_7_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Four_Pixels_NEON
    Blit_Three_Pixels_ARM
    Process_Next_Row_If_Needed .LBlit_7_Pixels_Per_Row

.LBlit_8_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Eight_Pixels_NEON
    Process_Next_Row_If_Needed .LBlit_8_Pixels_Per_Row

.LBlit_9_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Eight_Pixels_NEON
    Blit_Single_Pixel_ARM
    Process_Next_Row_If_Needed .LBlit_9_Pixels_Per_Row

.LBlit_10_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Eight_Pixels_NEON
    Blit_Two_Pixels_ARM
    Process_Next_Row_If_Needed .LBlit_10_Pixels_Per_Row

.LBlit_11_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Eight_Pixels_NEON
    Blit_Three_Pixels_ARM
    Process_Next_Row_If_Needed .LBlit_11_Pixels_Per_Row

.LBlit_12_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Eight_Pixels_NEON
    Blit_Four_Pixels_NEON
    Process_Next_Row_If_Needed .LBlit_12_Pixels_Per_Row

.LBlit_13_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Eight_Pixels_NEON
    Blit_Four_Pixels_NEON
    Blit_Single_Pixel_ARM
    Process_Next_Row_If_Needed .LBlit_13_Pixels_Per_Row

.LBlit_14_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Eight_Pixels_NEON
    Blit_Four_Pixels_NEON
    Blit_Two_Pixels_ARM
    Process_Next_Row_If_Needed .LBlit_14_Pixels_Per_Row

.LBlit_15_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Eight_Pixels_NEON
    Blit_Four_Pixels_NEON
    Blit_Three_Pixels_ARM
    Process_Next_Row_If_Needed .LBlit_15_Pixels_Per_Row

.LBlit_16_Pixels_Per_Row:
    Prefetch_Next_Data #256
    Blit_Eight_Pixels_NEON
    Blit_Eight_Pixels_NEON
    Process_Next_Row_If_Needed .LBlit_16_Pixels_Per_Row

.Lgreater_than_16:                              //do {
    mov             r7, r4                      //r7 = w = width

    pld             [r0, r1]
    pld             [r2, r3]

.LGT_16_Inner_Neon_Loop:                        //do {
    subs            r7, r7, #8
    cmp             r7, #8
    Blit_Eight_Pixels_NEON
    bge             .LGT_16_Inner_Neon_Loop     // } while ((w-=8) >= 8);

    cmp             r7, #0
    ble             .LGT_16_SkipInner_Arm_Loop

.LGT_16_Inner_Arm_Loop:                         //do {
    subs            r7, r7, #1
    Blit_Single_Pixel_ARM
    bne             .LGT_16_Inner_Arm_Loop      // } while (--w != 0);

.LGT_16_SkipInner_Arm_Loop:
    subs            r5, r5, #1
    add             r0,r0,r1                    //device = (uint32_t*)((char*)device + dstRB);
    add             r2,r2,r3                    //mask += maskRB;
    bne             .Lgreater_than_16           // } while (--height != 0);
    b               .LBlit_0_Pixel_Per_Row

.LBlit_0_Pixel_Per_Row:

    vpop            {q4-q7}
    pop             {r4, r5, r6, r7, r8, r9, r10, r11, r12, pc}
    nop

    .endfunc
    .end
