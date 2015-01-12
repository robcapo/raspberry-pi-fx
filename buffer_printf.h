/*
 * buffer_printf.h
 *
 *  Created on: Mar 11, 2014
 *      Author: Michael
 * 
 * @version 1.1 changed naming to match stdint and removed 2Buf from function names
 */

#ifndef BUFFER_PRINTF_H_
#define BUFFER_PRINTF_H_
#include <stdarg.h>
#include <stdint.h>

/**
 *  @brief printf implementation to char buffer
 *  
 *  Currently supports the following replacement flags:
 *  - @c %c char, replaces flag with specified ASCII char
 *  - @c %d signed 16 bit integer, replaces flag with specified int. 
 *    See Push_int16()
 *  - @c %e @c %f or @c %g float, replaces flag with specified float
 *    See PushFloat()
 *  - @c %s string, replaces flag with specified null terminated string
 *    See PushString()
 *  - @c %u unsigned 16 bit integer, replaces flag with specified unsigned int
 *    See Push_uint16()
 *  - @c %x 16 bit hex formated integer, replaces flag with 4 digit hex value
 *    See PushHex()
 *
 *  Example: 
 *  @code
 *  buffer_t tx;
 *  ...
 *  int16_t x = -1;
 *  char name[] = "Muhlbaier";
 *  Push_printf(&tx, "x = %d, hex - 0x%x, unsigned %u, name = %s);
 *  @endcode
 *  Would push to the buffer:
 *  "x = -1, hex - 0xFFFF, unsigned 65535, name = Muhlbaier"
 *  
 *  @param [in] buf Pointer to char buffer to print formatted string to
 *  @param [in] str Pointer to null terminated string with replacement flags
 *  @param [in] ... Variable argument list corresponding with replacement flags
 */
void Push_printf(buffer_t * buf, char * str, ...);

/**
 *  @brief vprintf implementation to char buffer
 *  
 *  Same as Push_printf() except with  a va_list pointer instead of an
 *  actual variable argument list. This allows other functions similar to
 *  Push_printf to be implemented.
 *
 *  For example:
 *  @code
 *  void LogStr(char * str, ...) {
 *     va_list vars;
 *     va_start(vars, str);
 *     // use Push_vprintf to log to LOG_BUF
 *     Push_vprintf(LOG_BUF, str, vars);
 *     va_end(vars);
 *  }
 *  @endcode
 *
 *  See Push_printf()
 *  
 *  @param [in] buf Pointer to char buffer to print formatted string to
 *  @param [in] str Pointer to null terminated string with replacement flags
 *  @param [in] ... Variable argument list corresponding with replacement flags
 */
void Push_vprintf(buffer_t * buf, char * str, va_list vars);

/**
 *  @brief Push unsigned integer to char buffer
 *  
 *  @param [in] buf Pointer to char buffer to print unsigned int to
 *  @param [in] x Unsigned integer to convert to text
 */
void Push_uint16(buffer_t * buf, uint16_t x);

/**
 *  @brief Push integer to char buffer
 *  
 *  Note this function is dependent on Push_uint16()
 *  
 *  @param [in] buf Pointer to char buffer to print int to
 *  @param [in] x Integer to convert to text
 */
void Push_int16(buffer_t * buf, int16_t x);

/**
 *  @brief Push unsigned long integer to char buffer
 *  
 *  @param [in] buf Pointer to char buffer to print unsigned long to
 *  @param [in] x Unsigned integer to convert to text
 */
void Push_uint32(buffer_t * buf, uint32_t x);

/**
 *  @brief Push long to char buffer
 *  
 *  Note this function is dependent on Push_uint32()
 *  
 *  @param [in] buf Pointer to char buffer to print int to
 *  @param [in] x Long to convert to text
 */
void Push_uint32(buffer_t * buf, uint32_t x);

/**
 *  @brief Push char array (string) to char buffer
 *  
 *  @param [in] buf Pointer to char buffer to print string to
 *  @param [in] str Pointer to null terminated char array (e.g. string)
 */
void PushStr(buffer_t * buf, char * str);

/**
 *  @brief Push 16 bit value to char buffer in hex format
 *  
 *  Will push four char's to the buffer, for example: A0F3
 *  
 *  @param [in] buf Pointer to char buffer to print hex formatted int to
 *  @param [in] x Integer to convert to hex
 */
void PushHex(buffer_t * buf, uint16_t x);

/**
 *  @brief Cheap implementation of float to char buffer
 *  
 *  Current implementation will format float as 0.000 by
 *  first printing out the integer portion of the float then
 *  multiplying the float by 1000 and subtracting the integer 
 *  portion x 1000 and printing that after the .
 *  
 *  @todo Do this better
 *  
 *  @param [in] buf Pointer to char buffer to print float to
 *  @param [in] x Float value to convert to text
 */
void PushFloat(buffer_t * buf, float x);

/** Reusable function to push a character to a terminal at a specific location.
 *
 * This function will only work when the buffer is used to communicate to a terminal
 * supporting the used features (e.g. PuTTY)
 *
 * Location 0,0 is the top left corner of the terminal
 *
 * @param buf pointer to buffer to send char to
 * @param c char to send
 * @param x zero base x location
 * @param y zero base y location
 */


#endif /* BUFFER_PRINTF_H_ */
