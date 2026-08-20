/*
 * Copyright (c) 2024-2026 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 *
 */

/**
 * @file doca_bitfield.h
 * @page doca_bitfield
 * @defgroup Bitfield operations for DOCA Types
 * @ingroup DOCACore
 * DOCA bitfield introduces bitfield operations on DOCA type that are common for many libraries.
 *
 * @{
 */

#ifndef DOCA_BITFIELD_H_
#define DOCA_BITFIELD_H_

#ifdef __linux__
#include <arpa/inet.h>
#else
#include <stdlib.h>
#include <intrin.h>
#endif

#include <doca_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux__
/**
 * DOCA_SHIFT() - get number of bits shifted
 *
 * @param _x: value
 */
#define DOCA_SHIFT(_x) (((_x) ? (__builtin_ffsll((_x)) - 1) : 0))

/**
 * DOCA_HTOBE16() - convert 16bit type to big endian from host endian
 *
 * @param _x: value
 */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DOCA_HTOBE16(_x) (__builtin_constant_p((_x)) ? __builtin_bswap16((_x)) : htons((_x)))
#else
#define DOCA_HTOBE16(_x) (_x)
#endif

/**
 * DOCA_HTOBE32() - convert 32bit type to big endian from host endian
 *
 * @param _x: value
 */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DOCA_HTOBE32(_x) (__builtin_constant_p((_x)) ? __builtin_bswap32((_x)) : htonl((_x)))
#else
#define DOCA_HTOBE32(_x) (_x)
#endif

/**
 * DOCA_HTOBE64() - convert 64bit type to big endian from host endian
 *
 * @param _x: value
 */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DOCA_HTOBE64(_x) __builtin_bswap64((_x))
#else
#define DOCA_HTOBE64(_x) (_x)
#endif

/**
 * DOCA_BETOH16() - convert 16bit to host endian from big endian
 *
 * @param _x: value
 */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DOCA_BETOH16(_x) (__builtin_constant_p((_x)) ? __builtin_bswap16((_x)) : ntohs((_x)))
#else
#define DOCA_BETOH16(_x) (_x)
#endif

/**
 * DOCA_BETOH32() - convert 32bit to host endian from big endian
 *
 * @param _x: value
 */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DOCA_BETOH32(_x) (__builtin_constant_p((_x)) ? __builtin_bswap32((_x)) : ntohl((_x)))
#else
#define DOCA_BETOH32(_x) (_x)
#endif

/**
 * DOCA_BETOH64() - convert 64bit to host endian from big endian
 *
 * @param _x: value
 */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DOCA_BETOH64(_x) __builtin_bswap64((_x))
#else
#define DOCA_BETOH64(_x) (_x)
#endif

#else /* __linux__ */

/**
 * __doca_builtin_ffsll() - internal implementation on windows, equal to gnu's __builtin_ffsll();
 *
 * @param x: value
 * @return: the least significant 1-bit index plus one; if x is zero, return zero.
 */
static int __doca_builtin_ffsll(long long x)
{
	unsigned long _f;

	if (_BitScanForward64(&_f, (unsigned long long)x)) {
		return (int)(_f + 1);
	}
	return 0;
}

/**
 * DOCA_SHIFT() - get number of bits shifted
 *
 * @param _x: value
 */

#define DOCA_SHIFT(_x) (__doca_builtin_ffsll((_x)) - 1)

/**
 * DOCA_HTOBE16() - convert 16bit type to big endian from host endian
 *
 * @param _x: value
 */
#define DOCA_HTOBE16(_x) _byteswap_ushort((_x))

/**
 * DOCA_HTOBE32() - convert 32bit type to big endian from host endian
 *
 * @param _x: value
 */
#define DOCA_HTOBE32(_x) _byteswap_ulong((_x))

/**
 * DOCA_BETOH16() - convert 16bit to host endian from big endian
 *
 * @param _x: value
 */
#define DOCA_BETOH16(_x) _byteswap_ushort((_x))

/**
 * DOCA_BETOH32() - convert 32bit to host endian from big endian
 *
 * @param _x: value
 */
#define DOCA_BETOH32(_x) _byteswap_ulong((_x))

#endif

/**
 * DOCA_U8_GENMASK() - generate continuous mask from @p _l bit to @p _h bit, return in host endian
 * For example, DOCA_U8_GENMASK(7, 4) -> 0xF0
 *
 * @param _l: lowest bit
 * @param _h: highest bit
 */
#define DOCA_U8_GENMASK(_h, _l) ((UINT8_MAX - (UINT8_C(1) << (_l)) + 1) & (UINT8_MAX >> (8 - 1 - (_h))))

/**
 * DOCA_U8P_GENMASK() - generate continuous mask from @p _l bit to @p _h bit,
 * put in @p _p pointed memory in host endian
 *
 * @param _l: lowest bit
 * @param _h: highest bit
 * @param _p: pointer of uint8_t
 */
#define DOCA_U8P_GENMASK(_h, _l, _p) \
	do { \
		*(uint8_t *)(_p) = DOCA_U8_GENMASK((_h), (_l)); \
	} while (0)

/**
 * DOCA_BE16_GENMASK() - generate continuous mask from @p _l bit to @p _h bit, return in big endian
 * For example, DOCA_BE16_GENMASK(11, 4) -> htons(0x0FF0)
 *
 * @param _l: lowest bit
 * @param _h: highest bit
 */
#define DOCA_BE16_GENMASK(_h, _l) \
	(DOCA_HTOBE16((UINT16_MAX - (UINT16_C(1) << (_l)) + 1) & (UINT16_MAX >> (16 - 1 - (_h)))))

/**
 * DOCA_BE16P_GENMASK() - generate continuous mask from @p _l bit to @p _h bit,
 * put in @p _p pointed memory in big endian
 *
 * @param _l: lowest bit
 * @param _h: highest bit
 * @param _p: pointer of doca_be16_t
 */
#define DOCA_BE16P_GENMASK(_h, _l, _p) \
	do { \
		*(doca_be16_t *)(_p) = DOCA_BE16_GENMASK((_h), (_l)); \
	} while (0)

/**
 * DOCA_BE32_GENMASK() - generate continuous mask from @p _l bit to @p _h bit, return in big endian
 * For example, DOCA_BE32_GENMASK(23, 4) -> htonl(0x00FFFFF0)
 *
 * @param _l: lowest bit
 * @param _h: highest bit
 */
#define DOCA_BE32_GENMASK(_h, _l) \
	(DOCA_HTOBE32((UINT32_MAX - (UINT32_C(1) << (_l)) + 1) & (UINT32_MAX >> (32 - 1 - (_h)))))

/**
 * DOCA_BE32P_GENMASK() - generate continuous mask from @p _l bit to @p _h bit,
 * put in @p _p pointed memory in big endian
 *
 * @param _l: lowest bit
 * @param _h: highest bit
 * @param _p: pointer of doca_be32_t
 */
#define DOCA_BE32P_GENMASK(_h, _l, _p) \
	do { \
		*(doca_be32_t *)(_p) = DOCA_BE32_GENMASK((_h), (_l)); \
	} while (0)

/**
 * DOCA_U8_SET() - set a bitfield segment in host endian and return
 *
 * @param _m: uint8_t, shifted mask defined field's width and position
 * @param _v: host endian, value to set in field
 *
 * DOCA_U8_SET() mask and shift up the value and return in uint8_t
 * The return value should be logical OR with other fields in register.
 */
#define DOCA_U8_SET(_m, _v) (((_v) << DOCA_SHIFT((_m))) & (_m))

/**
 * DOCA_U8P_SET() - set a bitfield segment in @p _p pointed uint8_t field
 *
 * @param _m: uint8_t, shifted mask defined field's width and position
 * @param _v: host endian, value to set in field
 * @param _p: uint8_t, pointer to field
 *
 * DOCA_U8P_SET() mask and shift up the value and logical OR with other fields in uint8_t
 */
#define DOCA_U8P_SET(_m, _v, _p) \
	do { \
		uint8_t _tmp = *(uint8_t *)(_p); \
\
		_tmp |= DOCA_U8_SET((_m), (_v)); \
		*(uint8_t *)(_p) = _tmp; \
	} while (0);

/**
 * DOCA_BE16_SET() - set a bitfield segment in big endian and return
 *
 * @param _m: doca_be16_t, shifted mask defined field's width and position
 * @param _v: host endian, value to set in field
 *
 * DOCA_BE16_SET() mask and shift up the value and return in doca_be16_t
 * The return value should be logical OR with other fields in register.
 */
#define DOCA_BE16_SET(_m, _v) ((DOCA_HTOBE16(((_v) << DOCA_SHIFT(DOCA_BETOH16((_m)))))) & (_m))

/**
 * DOCA_BE16P_SET() - set a bitfield segment in @p _p pointed doca_be16_t field
 *
 * @param _m: doca_be16_t, shifted mask defined field's width and position
 * @param _v: host endian, value to set in field
 * @param _p: doca_be16_t, pointer to field
 *
 * DOCA_BE16P_SET() mask and shift up the value and logical OR with other fields in doca_be16_t
 */
#define DOCA_BE16P_SET(_m, _v, _p) \
	do { \
		doca_be16_t _tmp = *(doca_be16_t *)(_p); \
\
		_tmp |= DOCA_BE16_SET((_m), (_v)); \
		*(doca_be16_t *)(_p) = _tmp; \
	} while (0);

/**
 * DOCA_BE32_SET() - set a bitfield segment in big endian and return
 *
 * @param _m: doca_be32_t, shifted mask defined field's width and position
 * @param _v: host endian, value to set in field
 *
 * DOCA_BE32_SET() mask and shift up the value and return in doca_be32_t
 * The return value should be logical OR with other fields in register.
 */
#define DOCA_BE32_SET(_m, _v) ((DOCA_HTOBE32(((_v) << DOCA_SHIFT(DOCA_BETOH32((_m)))))) & (_m))

/**
 * DOCA_BE32P_SET() - set a bitfield segment in @p _p pointed doca_be32_t field
 *
 * @param _m: doca_be32_t, shifted mask defined field's width and position
 * @param _v: host endian, value to set in field
 * @param _p: doca_be32_t, pointer to field
 *
 * DOCA_BE32P_SET() mask and shift up the value and logical OR with other fields in doca_be32_t
 */
#define DOCA_BE32P_SET(_m, _v, _p) \
	do { \
		doca_be32_t _tmp = *(doca_be32_t *)(_p); \
\
		_tmp |= DOCA_BE32_SET((_m), (_v)); \
		*(doca_be32_t *)(_p) = _tmp; \
	} while (0);

/**
 * DOCA_U8_GET() - get a bitfield segment value
 *
 * @param _m: uint8_t, shifted mask defined field's width and position
 * @param _f: uint8_t, entire register value
 *
 * DOCA_U8_GET() get the field value in host endian specified by @p _m from the register passed in as @p _f
 * by masking and shifting it down
 */
#define DOCA_U8_GET(_m, _f) (((_m) & (_f)) >> DOCA_SHIFT((_m)))

/**
 * DOCA_BE16_GET() - get a bitfield segment value
 *
 * @param _m: doca_be16_t, shifted mask defined field's width and position
 * @param _f: doca_be16_t, entire register value
 *
 * DOCA_BE16_GET() get the field value in host endian specified by @p _m from the register passed in as @p _f
 * by masking and shifting it down
 */
#define DOCA_BE16_GET(_m, _f) ((DOCA_BETOH16(((_m) & (_f))) >> DOCA_SHIFT(DOCA_BETOH16((_m)))))

/**
 * DOCA_BE32_GET() - get a bitfield segment value
 *
 * @param _m: doca_be32_t, shifted mask defined field's width and position
 * @param _f: doca_be32_t, entire register value
 *
 * DOCA_BE32_GET() get the field value in host endian specified by @p _m from the register passed in as @p _f
 * by masking and shifting it down
 */
#define DOCA_BE32_GET(_m, _f) ((DOCA_BETOH32(((_m) & (_f))) >> DOCA_SHIFT(DOCA_BETOH32((_m)))))

#ifdef __cplusplus
}
#endif

/** @} */
#endif /* DOCA_BITFIELD_H_ */
