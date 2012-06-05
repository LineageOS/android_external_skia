/*
 *
 * Copyright 2010 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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
 * @file    byte_swap.h
 * @brief   JPEG specific define.
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2011.12.01 : Create
 */

#ifndef BYTE_SWAP_H_
#define BYTE_SWAP_H_
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Swap 1st byte and 3rd byte
 *
 * @param dst
 *   dest address
 *
 * @param src
 *   src address
 *
 * @param size
 *   word(4bytes) size of src
 */
void csc_swap_1st_3rd_byte_neon(unsigned int *dst, unsigned int *src, unsigned int size);

/*
 * Swap 1st byte and 3rd byte and Change 1st byte to mask
 *
 * @param dst
 *   dest address
 *
 * @param src
 *   src address
 *
 * @param size
 *   word(4bytes) size of src
 *
 * @param value
 *   Changed value of 1st byte
 *   It should be 0x0 or 0xFF
 */
void csc_swap_1st_3rd_byte_mask_neon(unsigned int *dst, unsigned int *src, unsigned int size, unsigned char value);
#ifdef __cplusplus
}
#endif
#endif /*BYTE_SWAP_H_*/
