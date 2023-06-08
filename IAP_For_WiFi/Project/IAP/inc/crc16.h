/*
 *	crc16.h - CRC-16 routine
 *
 * Implements the standard CRC-16:
 *   Width 16
 *   Poly  0x8005 (x^16 + x^15 + x^2 + 1)
 *   Init  0
 *
 * Copyright (c) 2005 Ben Gardner <bgardner@wabtec.com>
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2. See the file COPYING for more details.
 */

#ifndef __CRC16_H
#define __CRC16_H

//#define USE_CRC16_1
//#define USE_CRC16_2
//#define USE_CRC16_3
#define USE_CRC16_CCITT
#ifdef 	USE_CRC16_CCITT
extern unsigned  CRC16_ccitt(const char *buf, unsigned len);
#endif
#ifdef 	USE_CRC16_1
extern uint16 const crc16_table[256]; 
uint16 CRC16_1(uint16 crc, uint8 const *buffer, size_t len);
#endif
#ifdef 	USE_CRC16_2
extern uint16 const crc16_table[256]; 
extern uint16 crc16(uint16 crc, const uint8 *buffer, size_t len);
#endif
#ifdef 	USE_CRC16_3
extern uint16 const crc16_table[256]; 
extern uint16 crc16(uint16 crc, const uint8 *buffer, size_t len);
#endif


#endif /* __CRC16_H */

