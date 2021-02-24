#ifndef __HAL_DEFS_H_
#define __HAL_DEFS_H_

/* takes a byte out of a uint32_t : var - uint32_t,  ByteNum - byte to take out (0 - 3) */
#define BREAK_UINT32( var, ByteNum ) \
          (uint8_t)((uint32_t)(((var) >>((ByteNum) * 8)) & 0x00FF))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((uint32_t)((uint32_t)((Byte0) & 0x00FF) \
          + ((uint32_t)((Byte1) & 0x00FF) << 8) \
          + ((uint32_t)((Byte2) & 0x00FF) << 16) \
          + ((uint32_t)((Byte3) & 0x00FF) << 24)))

#define BUILD_UINT16(loByte, hiByte) \
          ((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)	/* 获取uint16_t类型数据高位 */
#define LO_UINT16(a) ((a) & 0xFF)			/* 获取uint16_t类型数据低位 */

#define BUILD_UINT8(hiByte, loByte) \
          ((uint8_t)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))
  
#define HI_UINT8(a) (((a) >> 4) & 0x0F)

#define LO_UINT8(a) ((a) & 0x0F)

#define BCD_to_HEC(b)	(uint8_t)(((b)>>4)*10+((b)&0x0F))	/* BCD码转换为十六进制 */
    
#define HEC_to_BCD(h) 	(uint8_t)((((h)/10)<<4)|((h)%10))  /* 十六进制转换为BCD码 */
    
#define GET_COMPLEMENT(b)	(~(b)+1)							/* 获取一个整型数的补码 */

#define ARRAYNUM(arr_name)	(size_t)(sizeof(arr_name)/sizeof(arr_name[0]))	/* 计算数组元素个数 */

#define NUM_SET_BIT(dat, offset)     dat |= (1 << offset)       /* 置位某位为1 */

#define NUM_RESET_BIT(dat, offset)   dat &= ~(1 << offset)      /* 复位某位为0 */

#define NUM_GET_BIT(dat, offset)     ((dat >> offset) &0x01)    /* 获取某位 */

#define ASCII_TO_UINT8(c)		 ((c) - '0')

#endif
