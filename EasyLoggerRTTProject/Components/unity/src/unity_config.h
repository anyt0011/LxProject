#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

#include "SEGGER_RTT.h"
/* By default, Unity prints its results to `stdout` as it runs. This works
 * perfectly fine in most situations where you are using a native compiler for
 * testing. It works on some simulators as well so long as they have `stdout`
 * routed back to the command line. There are times, however, where the
 * simulator will lack support for dumping results or you will want to route
 * results elsewhere for other reasons. In these cases, you should define the
 * `UNITY_OUTPUT_CHAR` macro. This macro accepts a single character at a time
 * (as an `int`, since this is the parameter type of the standard C `putchar`
 * function most commonly used). You may replace this with whatever function
 * call you like.
 *
 * Example:
 * Say you are forced to run your test suite on an embedded processor with no
 * `stdout` option. You decide to route your test result output to a custom
 * serial `RS232_putc()` function you wrote like thus:
 */
#define UNITY_OUTPUT_CHAR(a)         SEGGER_RTT_Write(0,(uint8_t[]){(uint8_t)a},1)           
/* #define UNITY_OUTPUT_CHAR_HEADER_DECLARATION    RS232_putc(int) */
/* #define UNITY_OUTPUT_FLUSH()                    RS232_flush() */
/* #define UNITY_OUTPUT_FLUSH_HEADER_DECLARATION   RS232_flush(void) */
/* #define UNITY_OUTPUT_START()                    RS232_config(115200,1,8,0) */
/* #define UNITY_OUTPUT_COMPLETE()                 RS232_close() */



#endif /* UNITY_CONFIG_H */
