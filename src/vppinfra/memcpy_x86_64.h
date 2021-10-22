/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2021 Damjan Marion
 */

#ifndef included_clib_memcpy_x86_64_h
#define included_clib_memcpy_x86_64_h
#ifdef __x86_64__

#include <vppinfra/clib.h>

static_always_inline void
clib_memcpy_const_le32 (u8 *dst, u8 *src, size_t n)
{
  switch (n)
    {
    case 1:
      *(u8 *) dst = *(u8 *) src;
      break;
    case 2:
      *(u16u *) dst = *(u16u *) src;
      break;
    case 3:
      *(u16u *) dst = *(u16u *) src;
      *((u8 *) dst + 2) = *((u8 *) src + 2);
      break;
    case 4:
      *(u32u *) dst = *(u32u *) src;
      break;
    case 5:
      *(u32u *) dst = *(u32u *) src;
      *((u8 *) dst + 4) = *((u8 *) src + 4);
      break;
    case 6:
      *(u32u *) dst = *(u32u *) src;
      *((u16u *) dst + 2) = *((u16u *) src + 2);
      break;
    case 7:
      *(u32u *) dst = *(u32u *) src;
      *((u32u *) (dst + 3)) = *((u32u *) (src + 3));
      break;
    case 8:
      *(u64u *) dst = *(u64u *) src;
      break;
    case 9:
      *(u64u *) dst = *(u64u *) src;
      *(dst + 8) = *(src + 8);
      break;
    case 10:
      *(u64u *) dst = *(u64u *) src;
      *((u16u *) (dst + 8)) = *((u16u *) (src + 8));
      break;
    case 11:
    case 12:
      *(u64u *) dst = *(u64u *) src;
      *((u32u *) (dst + n - 4)) = *((u32u *) (src + n - 4));
      break;
    case 13:
    case 14:
    case 15:
      *(u64u *) dst = *(u64u *) src;
      *((u64u *) (dst + n - 8)) = *((u64u *) (src + n - 8));
      break;
    case 16:
      *(u8x16u *) dst = *(u8x16u *) src;
      break;
    case 17:
      *(u8x16u *) dst = *(u8x16u *) src;
      *(dst + 16) = *(src + 16);
      break;
    case 18:
      *(u8x16u *) dst = *(u8x16u *) src;
      *((u16u *) (dst + 16)) = *((u16u *) (src + 16));
      break;
    case 20:
      *(u8x16u *) dst = *(u8x16u *) src;
      *((u32u *) (dst + 16)) = *((u32u *) (src + 16));
      break;
    case 24:
      *(u8x16u *) dst = *(u8x16u *) src;
      *((u64u *) (dst + 16)) = *((u64u *) (src + 16));
      break;
    default:
      *(u8x16u *) dst = *(u8x16u *) src;
      *((u8x16u *) (dst + n - 16)) = *((u8x16u *) (src + n - 16));
      break;
    }
}

static_always_inline void
clib_memcpy_const_le64 (u8 *dst, u8 *src, size_t n)
{
  if (n < 32)
    {
      clib_memcpy_const_le32 (dst, src, n);
      return;
    }

#if defined(CLIB_HAVE_VEC256)
  switch (n)
    {
    case 32:
      *(u8x32u *) dst = *(u8x32u *) src;
      break;
    case 33:
      *(u8x32u *) dst = *(u8x32u *) src;
      *(dst + 32) = *(src + 32);
      break;
    case 34:
      *(u8x32u *) dst = *(u8x32u *) src;
      *((u16u *) (dst + 32)) = *((u16u *) (src + 32));
      break;
    case 36:
      *(u8x32u *) dst = *(u8x32u *) src;
      *((u32u *) (dst + 32)) = *((u32u *) (src + 32));
      break;
    case 40:
      *(u8x32u *) dst = *(u8x32u *) src;
      *((u64u *) (dst + 32)) = *((u64u *) (src + 32));
      break;
    case 48:
      *(u8x32u *) dst = *(u8x32u *) src;
      *((u8x16u *) (dst + 32)) = *((u8x16u *) (src + 32));
      break;
    default:
      *(u8x32u *) dst = *(u8x32u *) src;
      *((u8x32u *) (dst + n - 32)) = *((u8x32u *) (src + n - 32));
      break;
    }
#else
  while (n > 31)
    {
      u8x16u *dv = (u8x16u *) dst;
      u8x16u *sv = (u8x16u *) src;
      dv[0] = sv[0];
      dv[1] = sv[1];
      dst += 32;
      src += 32;
      n -= 32;
    }
  clib_memcpy_const_le32 (dst, src, n);
#endif
}

static_always_inline void
clib_memcpy_x86_64_const (u8 *dst, u8 *src, size_t n)
{
#if defined(CLIB_HAVE_VEC512)
  while (n > 128)
    {
      u8x64u *dv = (u8x64u *) dst;
      u8x64u *sv = (u8x64u *) src;
      dv[0] = sv[0];
      dst += 64;
      src += 64;
      n -= 64;
    }

  if (n < 64)
    {
      clib_memcpy_const_le64 (dst, src, n);
      return;
    }

  switch (n)
    {
    case 64:
      *(u8x64u *) dst = *(u8x64u *) src;
      break;
    case 65:
      *(u8x64u *) dst = *(u8x64u *) src;
      *(dst + 64) = *(src + 64);
      break;
    case 66:
      *(u8x64u *) dst = *(u8x64u *) src;
      *((u16u *) (dst + 64)) = *((u16u *) (src + 64));
      break;
    case 68:
      *(u8x64u *) dst = *(u8x64u *) src;
      *((u32u *) (dst + 64)) = *((u32u *) (src + 64));
      break;
    case 72:
      *(u8x64u *) dst = *(u8x64u *) src;
      *((u64u *) (dst + 64)) = *((u64u *) (src + 64));
      break;
    case 80:
      *(u8x64u *) dst = *(u8x64u *) src;
      *((u8x16u *) (dst + 64)) = *((u8x16u *) (src + 64));
      break;
    case 96:
      *(u8x64u *) dst = *(u8x64u *) src;
      *((u8x32u *) (dst + 64)) = *((u8x32u *) (src + 64));
      break;
    default:
      *(u8x64u *) dst = *(u8x64u *) src;
      *((u8x64u *) (dst + n - 64)) = *((u8x64u *) (src + n - 64));
      break;
    }
#elif defined(CLIB_HAVE_VEC256)
  while (n > 64)
    {
      u8x32u *dv = (u8x32u *) dst;
      u8x32u *sv = (u8x32u *) src;
      dv[0] = sv[0];
      dst += 32;
      src += 32;
      n -= 32;
    }
  clib_memcpy_const_le64 (dst, src, n);
#else
  while (n > 32)
    {
      u8x16u *dv = (u8x16u *) dst;
      u8x16u *sv = (u8x16u *) src;
      dv[0] = sv[0];
      dst += 16;
      src += 16;
      n -= 16;
    }
  clib_memcpy_const_le32 (dst, src, n);
#endif
}

static_always_inline void *
clib_memcpy_x86_64 (void *restrict dst, const void *restrict src, size_t n)
{
#if defined(CLIB_HAVE_VEC512)
  u8x64u *dv = (u8x64u *) dst, *sv = (u8x64u *) src;
  const u8 vec_bytes = 64;
  const u16 block_bytes = 512;
#elif defined(CLIB_HAVE_VEC256)
  u8x32u *dv = (u8x32u *) dst, *sv = (u8x32u *) src;
  const u8 vec_bytes = 32;
#else
  u8x16u *dv = (u8x16u *) dst, *sv = (u8x16u *) src;
  const u8 vec_bytes = 16;
  const u16 block_bytes = 256;
#endif
  u8 *d = (u8 *) dst, *s = (u8 *) src;
  u64 off;

  /* emit minimal number of instructions for cases where n is compile-time
   * constant */
  if (COMPILE_TIME_CONST (n))
    {
      clib_memcpy_x86_64_const (d, s, n);
      goto done;
    }

#if 1
  /* copy less than largest vector register size */
  if (PREDICT_TRUE (n < vec_bytes))
    {
#if defined(CLIB_HAVE_VEC512)
      if (n > 32)
	{
	  *(u8x32u *) d = *(u8x32u *) s;
	  *(u8x32u *) (d + n - 32) = *(u8x32u *) (s + n - 32);
	  goto done;
	}
#endif
#if defined(CLIB_HAVE_VEC256_MASK_LOAD_STORE)
      u32 mask = pow2_mask (n);
      u8x32_mask_store (u8x32_mask_load_zero (s, mask), d, mask);
#else
      if (PREDICT_TRUE (n >= 8))
	{
#ifdef CLIB_HAVE_VEC256 /* AVX2 only - no mask load/store */
	  if (n >= 16)
	    {
	      *(u8x16u *) d = *(u8x16u *) s;
	      *(u8x16u *) (d + n - 16) = *(u8x16u *) (s + n - 16);
	      goto done;
	    }
#endif
	  *(u64u *) d = *(u64u *) s;
	  *(u64u *) (d + n - 8) = *(u64u *) (s + n - 8);
	  goto done;
	}

      if (PREDICT_TRUE (n >= 2))
	{
	  if (n >= 4)
	    {
	      *(u32u *) d = *(u32u *) s;
	      *(u32u *) (d + n - 4) = *(u32u *) (s + n - 4);
	      goto done;
	    }

	  *(u16u *) d = *(u16u *) s;
	  *(u16u *) (d + n - 2) = *(u16u *) (s + n - 2);
	  goto done;
	}

      *d = *s;
#endif
      goto done;
    }

#ifdef CLIB_HAVE_VEC512
  if (PREDICT_FALSE (n < block_bytes))
    goto last;
#elif defined(CLIB_HAVE_VEC256)
#else
  if (PREDICT_FALSE (n < block_bytes))
    goto last;
#endif

  if (n > 128 && (off = (uword) d & (vec_bytes - 1)))
    {
      /* dst pointer is not aligned */
      off = vec_bytes - off;
      dv[0] = sv[0];
      d += off;
      s += off;
      n -= off;
    }
#endif

#ifdef CLIB_HAVE_VEC512
  while (PREDICT_TRUE (n >= block_bytes))
    {
      u8x64 zmm0, zmm1, zmm2;
      asm volatile(
	/* copy 512 bytes using 64-byte load/stores, n -= 512,
	 * dst += 512, src += 512 */
	"vmovdqu8	(%[src]), %[zmm0]	\n\t" /* load 0 */
	"vmovdqu8	0x40(%[src]), %[zmm1]	\n\t" /* load 1 */
	"vmovdqu8	0x80(%[src]), %[zmm2]	\n\t" /* load 2 */
	"subq		$0x200,%[n]		\n\t" /* n -= 512 */
	"vmovdqa64	%[zmm0], (%[dst])	\n\t" /* store 0 */
	"vmovdqu8	0xc0(%[src]), %[zmm0]	\n\t" /* load 3 */
	"vmovdqa64	%[zmm1], 0x40(%[dst])	\n\t" /* store 1 */
	"vmovdqu8	0x100(%[src]), %[zmm1]	\n\t" /* load 4 */
	"vmovdqa64	%[zmm2], 0x80(%[dst])	\n\t" /* store 2 */
	"vmovdqu8	0x140(%[src]), %[zmm2]	\n\t" /* load 5 */
	"vmovdqa64	%[zmm0], 0xc0(%[dst])	\n\t" /* store 3 */
	"vmovdqu8	0x180(%[src]), %[zmm0]	\n\t" /* load 6 */
	"vmovdqa64	%[zmm1], 0x100(%[dst])	\n\t" /* store 4 */
	"vmovdqu8	0x1c0(%[src]), %[zmm1]	\n\t" /* load 7 */
	"addq		$0x200,%[src]		\n\t" /* src += 512  */
	"vmovdqa64	%[zmm2], 0x140(%[dst])	\n\t" /* store 5 */
	"vmovdqa64	%[zmm0], 0x180(%[dst])	\n\t" /* store 6 */
	"vmovdqa64	%[zmm1], 0x1c0(%[dst])	\n\t" /* store 7 */
	"addq	$0x200,%[dst]		\n\t"	      /* dst += 512 */
	: [zmm0] "=&x"(zmm0), [zmm1] "=&x"(zmm1), [zmm2] "=&x"(zmm2),
	  [dst] "+D"(d), [src] "+S"(s), [n] "+r"(n)
	:
	: "memory");
    }
#elif defined(CLIB_HAVE_VEC256)
  if (1)
    {
      u8x32 ymm0, ymm1, ymm2, ymm3;
      u64 off, r0, r1;
      asm volatile(
	/* set offset to 0 */
	"		xor		%[off], %[off]			\n\t"
	/* skip main loop if number of bytes is < 256 */
	"		cmp		$0xff,%[n]			\n\t"
	"		jbe		6f				\n\t"
	/* calculate number of bytes in 256-byte blocks */
	"5:								\n\t"
	"		mov		%[n], %[r0]			\n\t"
	"		xorb		%b[r0], %b[r0]			\n\t"
	"1:								\n\t"
	/* main 256-byte copy loop */
	"		vmovdqu		0x00(%[src],%[off]), %[ymm0]	\n\t"
	"		vmovdqu		0x20(%[src],%[off]), %[ymm1]	\n\t"
	"		vmovdqu		0x40(%[src],%[off]), %[ymm2]	\n\t"
	"		vmovdqu		0x60(%[src],%[off]), %[ymm3]	\n\t"
	"		vmovdqa		%[ymm0], 0x00(%[dst],%[off])	\n\t"
	"		vmovdqa		%[ymm1], 0x20(%[dst],%[off])	\n\t"
	"		vmovdqa		%[ymm2], 0x40(%[dst],%[off])	\n\t"
	"		vmovdqa		%[ymm3], 0x60(%[dst],%[off])	\n\t"
	"		vmovdqu		0x80(%[src],%[off]), %[ymm0]	\n\t"
	"		vmovdqu		0xa0(%[src],%[off]), %[ymm1]	\n\t"
	"		vmovdqu		0xc0(%[src],%[off]), %[ymm2]	\n\t"
	"		vmovdqu		0xe0(%[src],%[off]), %[ymm3]	\n\t"
	"		vmovdqa		%[ymm0], 0x80(%[dst],%[off])	\n\t"
	"		vmovdqa		%[ymm1], 0xa0(%[dst],%[off])	\n\t"
	"		vmovdqa		%[ymm2], 0xc0(%[dst],%[off])	\n\t"
	"		vmovdqa		%[ymm3], 0xe0(%[dst],%[off])	\n\t"
	"		addq		$0x100,%[off]			\n\t"
	"		cmp		%[r0], %[off]			\n\t"
	"		jne		1b				\n\t"
	"2:								\n\t"

	/* check if there is more bytes to copy (256 > n > 0) */
	"		cmp		%[off], %[n]			\n\t"
	"		je		4f				\n\t"

#if 1
	"		cmp		$0x20,%b[n]			\n\t"
	"		jbe		9f				\n\t"
	"6:								\n\t"
	"		vmovdqu		0x00(%[src],%[off]), %[ymm0]	\n\t"
	"		vmovdqu		%[ymm0], 0x00(%[dst],%[off])	\n\t"
	"		cmp		$0x40,%b[n]			\n\t"
	"		jbe		9f				\n\t"
	"		vmovdqu 0x20(%[src],%[off]),%[ymm0]		\n\t"
	"		vmovdqu %[ymm0], 0x20(%[dst],%[off])		\n\t"
	"		cmp		$0x80,%b[n]			\n\t"
	"		jbe		8f				\n\t"
	"		vmovdqu 0x40(%[src],%[off]),%[ymm0]		\n\t"
	"		vmovdqu %[ymm0], 0x40(%[dst],%[off])		\n\t"
	"		vmovdqu 0x60(%[src],%[off]),%[ymm0]		\n\t"
	"		vmovdqu %[ymm0], 0x60(%[dst],%[off])		\n\t"
	"		vmovdqu -0x80(%[src],%[n]),%[ymm0]		\n\t"
	"		vmovdqu %[ymm0], -0x80(%[dst],%[n])		\n\t"
	"		vmovdqu -0x60(%[src],%[n]),%[ymm0]		\n\t"
	"		vmovdqu %[ymm0], -0x60(%[dst],%[n])		\n\t"
	"8:								\n\t"
	"		vmovdqu -0x40(%[src],%[n]),%[ymm0]		\n\t"
	"		vmovdqu %[ymm0], -0x40(%[dst],%[n])		\n\t"
	"9:								\n\t"
	"		vmovdqu -0x20(%[src],%[n]),%[ymm0]		\n\t"
	"		vmovdqu %[ymm0], -0x20(%[dst],%[n])		\n\t"
	: [ymm0] "=&x"(ymm0), [ymm1] "=&x"(ymm1), [ymm2] "=&x"(ymm2),
	  [ymm3] "=&x"(ymm3), [dst] "+D"(d), [src] "+S"(s), [n] "+r"(n),
	  [off] "+&r"(off), [r0] "+&r"(r0)
	:
	: "memory");
#else
	/* Calculate jump offset.
	 * VEX encoded unaligned move with base, offset and 32 bit
	 * displacement takes 9 bytes so we need to jump back 18 bytes
	 * for each 32-byte load/store needed
	 * Formula is:
	 *   - bits 5-7 from n represent number od 32-byte blocks
	 *   - ((n & 0xe0) >> 5) * 18 =
	 *     ((n & 0xe0) >> 4) * 9
	 *   - use LEA 8 * x + x to multiply by 9
	 */
	"		movq		%[n], %[r1]			\n\t"
	"		andq		$0xe0, %[r1]			\n\t"
	"		shrq		$4, %[r1]			\n\t"
	"		leaq		(%[r1],%[r1],8), %[r1]		\n\t"

	/* calculate start jump offset from RIP and subtract jump offset
	 * calculated above
	 */
	"		lea		3f(%%rip), %[r0]		\n\t"
	"		subq		%[r1], %[r0]			\n\t"
	"		jmp		*%[r0]				\n\t"
	"		vmovdqu		0xc0(%[src],%[off]), %[ymm0]	\n\t"
	"		vmovdqu		%[ymm0], 0xc0(%[dst],%[off])	\n\t"
	"		vmovdqu		0xa0(%[src],%[off]), %[ymm0]	\n\t"
	"		vmovdqu		%[ymm0], 0xa0(%[dst],%[off])	\n\t"
	"		vmovdqu		0x80(%[src],%[off]), %[ymm0]	\n\t"
	"		vmovdqu		%[ymm0], 0x80(%[dst],%[off])	\n\t"
	"%{disp32%}	vmovdqu		0x60(%[src],%[off]), %[ymm0]	\n\t"
	"%{disp32%}	vmovdqu		%[ymm0], 0x60(%[dst],%[off])	\n\t"
	"%{disp32%}	vmovdqu		0x40(%[src],%[off]), %[ymm0]	\n\t"
	"%{disp32%}	vmovdqu		%[ymm0], 0x40(%[dst],%[off])	\n\t"
	"%{disp32%}	vmovdqu		0x20(%[src],%[off]), %[ymm0]	\n\t"
	"%{disp32%}	vmovdqu		%[ymm0], 0x20(%[dst],%[off])	\n\t"
	"%{disp32%}	vmovdqu		0x00(%[src],%[off]), %[ymm0]	\n\t"
	"%{disp32%}	vmovdqu		%[ymm0], 0x00(%[dst],%[off])	\n\t"
	"3:								\n\t"
	/* copy bytes from n-32 to n-1 - this code assumes that n is
	 * always >= 32
	 * */
	"		vmovdqu		-0x20(%[src],%[n]), %[ymm0]	\n\t"
	"		vmovdqu		%[ymm0], -0x20(%[dst],%[n])	\n\t"
	"4:								\n\t"
	: [ymm0] "=&x"(ymm0), [ymm1] "=&x"(ymm1), [ymm2] "=&x"(ymm2),
	  [ymm3] "=&x"(ymm3), [dst] "+D"(d), [src] "+S"(s), [n] "+r"(n),
	  [off] "+&r"(off), [r0] "+&r"(r0), [r1] "+&r"(r1)
	:
	: "memory");
#endif

      return dst;
    }
#else
  while (PREDICT_TRUE (n >= block_bytes))
    {
      u8x16 xmm0, xmm1, xmm2;
      asm volatile(
	/* copy 256 bytes using 32-byte load/stores, n -= 256,
	 * dst += 256 , src += 256 */
	"vmovdqu	(%[src]), %[xmm0]	\n\t" /* load 0, 4 */
	"vmovdqu	0x40(%[src]), %[xmm1]	\n\t"

	"subq		$0x100, %[n]		\n\t" /* n -= 256 */

	"vmovdqa	%[xmm0], (%[dst])	\n\t" /* load 1-3, store 0-3 */
	"vmovdqu	0x10(%[src]), %[xmm0]	\n\t"
	"vmovdqa	%[xmm0], 0x10(%[dst])	\n\t"
	"vmovdqu	0x20(%[src]), %[xmm0]	\n\t"
	"vmovdqa	%[xmm0], 0x20(%[dst])	\n\t"
	"vmovdqu	0x30(%[src]), %[xmm0]	\n\t"
	"vmovdqa	%[xmm0], 0x30(%[dst])	\n\t"

	"vmovdqu	0x80(%[src]), %[xmm2]	\n\t" /* load 8 */

	"vmovdqa	%[xmm1], 0x40(%[dst])	\n\t" /* load 5-7, store 4-7 */
	"vmovdqu	0x50(%[src]), %[xmm1]	\n\t"
	"vmovdqa	%[xmm1], 0x50(%[dst])	\n\t"
	"vmovdqu	0x60(%[src]), %[xmm1]	\n\t"
	"vmovdqa	%[xmm1], 0x60(%[dst])	\n\t"
	"vmovdqu	0x70(%[src]), %[xmm1]	\n\t"
	"vmovdqa	%[xmm1], 0x70(%[dst])	\n\t"

	"vmovdqu	0xc0(%[src]), %[xmm0]	\n\t" /* load 12 */

	"vmovdqa	%[xmm2], 0x80(%[dst])	\n\t" /* load 9-11, st 8-11 */
	"vmovdqu	0x90(%[src]), %[xmm2]	\n\t"
	"vmovdqa	%[xmm2], 0x90(%[dst])	\n\t"
	"vmovdqu	0xa0(%[src]), %[xmm2]	\n\t"
	"vmovdqa	%[xmm2], 0xa0(%[dst])	\n\t"
	"vmovdqu	0xb0(%[src]), %[xmm2]	\n\t"
	"vmovdqa	%[xmm2], 0xb0(%[dst])	\n\t"

	"vmovdqa	%[xmm0], 0xc0(%[dst])	\n\t" /* store 12 */

	"vmovdqu	0xd0(%[src]), %[xmm0]	\n\t" /* load 13-15 */
	"vmovdqu	0xe0(%[src]), %[xmm1]	\n\t"
	"vmovdqu	0xf0(%[src]), %[xmm2]	\n\t"

	"addq		$0x100, %[src]		\n\t" /* src += 256 */

	"vmovdqa	%[xmm0], 0xd0(%[dst])	\n\t" /* store 13-15 */
	"vmovdqa	%[xmm1], 0xe0(%[dst])	\n\t"
	"vmovdqa	%[xmm2], 0xf0(%[dst])	\n\t"

	"addq	$0x100, %[dst]		\n\t" /* dst += 256 */

	: [xmm0] "=&x"(xmm0), [xmm1] "=&x"(xmm1), [xmm2] "=&x"(xmm2),
	  [dst] "+D"(d), [src] "+S"(s), [n] "+r"(n)
	:
	: "memory");
    }
#endif


#ifdef CLIB_HAVE_VEC512
last:
  if (PREDICT_TRUE (n))
    {
      off = n - vec_bytes;
      u64 r0;
      u64 skip = ((n >> 6) * 16);
      u8x64 zmm0;
      asm volatile(
	"lea		1f(%%rip), %[r0]	\n\t"
	"subq		%[skip], %[r0]		\n\t"
	"jmp		*%[r0]			\n\t"

	".align 16				\n\t"
	"vmovdqu8	0x180(%[src]), %[zmm0];	\n\t"
	"vmovdqu8	%[zmm0], 0x180(%[dst]);	\n\t"
	".align 16				\n\t"
	"vmovdqu8	0x140(%[src]), %[zmm0];	\n\t"
	"vmovdqu8	%[zmm0], 0x140(%[dst]);	\n\t"
	".align 16				\n\t"
	"vmovdqu8	0x100(%[src]), %[zmm0];	\n\t"
	"vmovdqu8	%[zmm0], 0x100(%[dst]);	\n\t"
	".align 16				\n\t"
	"vmovdqu8	0xc0(%[src]), %[zmm0];	\n\t"
	"vmovdqu8	%[zmm0], 0xc0(%[dst]);	\n\t"
	".align 16				\n\t"
	"vmovdqu8	0x80(%[src]), %[zmm0];	\n\t"
	"vmovdqu8	%[zmm0], 0x80(%[dst]);	\n\t"
	".align 16				\n\t"
	"vmovdqu8	0x40(%[src]), %[zmm0];	\n\t"
	"vmovdqu8	%[zmm0], 0x40(%[dst]);	\n\t"
	".align 16				\n\t"
	"%{disp8%} "
	"vmovdqu8	0x00(%[src]), %[zmm0];	\n\t"
	"%{disp8%} "
	"vmovdqu8	%[zmm0], 0x00(%[dst]);	\n\t"
	".align 16				\n\t"
	"1:					\n\t"
	"addq		%[off], %[src]		\n\t"
	"vmovdqu8	(%[src]), %[zmm0]	\n\t"
	"addq		%[off], %[dst]		\n\t"
	"vmovdqu8	%[zmm0], (%[dst])	\n\t"

	: [r0] "=&r"(r0), [zmm0] "=&x"(zmm0)
	: [dst] "D"(d), [src] "S"(s), [skip] "r"(skip), [off] "r"(off)
	: "memory");
    }

#elif defined(CLIB_HAVE_VEC256)
#else
last:
  if (PREDICT_TRUE (n))
    {
      off = n - vec_bytes;
      u64 r0;
      u64 skip = n & ~0x0f;
      u8x16 xmm0;
      asm volatile(
	"lea 1f(%%rip), %[r0]\n"
	"subq %[skip], %[r0]\n"
	"jmp *%[r0]\n"

	"vmovdqu 0xe0(%[src]), %[xmm0];\n"
	"vmovdqu %[xmm0], 0xe0(%[dst]);\n"
	"vmovdqu 0xd0(%[src]), %[xmm0];\n"
	"vmovdqu %[xmm0], 0xd0(%[dst]);\n"
	"vmovdqu 0xc0(%[src]), %[xmm0];\n"
	"vmovdqu %[xmm0], 0xc0(%[dst]);\n"

	"vmovdqu 0xb0(%[src]), %[xmm0];\n"
	"vmovdqu %[xmm0], 0xb0(%[dst]);\n"
	"vmovdqu 0xa0(%[src]), %[xmm0];\n"
	"vmovdqu %[xmm0], 0xa0(%[dst]);\n"

	"vmovdqu 0x90(%[src]), %[xmm0];\n"
	"vmovdqu %[xmm0], 0x90(%[dst]);\n"
	"vmovdqu 0x80(%[src]), %[xmm0];\n"
	"vmovdqu %[xmm0], 0x80(%[dst]);\n"

	"%{disp32%} vmovdqu 0x70(%[src]), %[xmm0];\n"
	"%{disp32%} vmovdqu %[xmm0], 0x70(%[dst]);\n"
	"%{disp32%} vmovdqu 0x60(%[src]), %[xmm0];\n"
	"%{disp32%} vmovdqu %[xmm0], 0x60(%[dst]);\n"

	"%{disp32%} vmovdqu 0x50(%[src]), %[xmm0];\n"
	"%{disp32%} vmovdqu %[xmm0], 0x50(%[dst]);\n"
	"%{disp32%} vmovdqu 0x40(%[src]), %[xmm0];\n"
	"%{disp32%} vmovdqu %[xmm0], 0x40(%[dst]);\n"

	"%{disp32%} vmovdqu 0x30(%[src]), %[xmm0];\n"
	"%{disp32%} vmovdqu %[xmm0], 0x30(%[dst]);\n"
	"%{disp32%} vmovdqu 0x20(%[src]), %[xmm0];\n"
	"%{disp32%} vmovdqu %[xmm0], 0x20(%[dst]);\n"

	"%{disp32%} vmovdqu 0x10(%[src]), %[xmm0];\n"
	"%{disp32%} vmovdqu %[xmm0], 0x10(%[dst]);\n"
	"%{disp32%} vmovdqu (%[src]), %[xmm0];\n"
	"%{disp32%} vmovdqu %[xmm0], (%[dst]);\n"

	"1:\n"
	"addq %[off], %[src]\n"
	"vmovdqu (%[src]), %[xmm0]\n"
	"addq %[off], %[dst]\n"
	"vmovdqu %[xmm0], (%[dst])\n"

	: [r0] "=&r"(r0), [xmm0] "=&x"(xmm0)
	: [dst] "D"(d), [src] "S"(s), [skip] "r"(skip), [off] "r"(off)
	: "memory");
    }
#endif
done:
  return dst;
}

#endif
#endif
