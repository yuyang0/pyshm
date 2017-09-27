/*Time-stamp: <2013-01-15 11:28:12 by Yu Yang>
 * =======================================================================
 *       Filename:  log.h
 *        Created:  2015-04-09 16:23:45
 *       Compiler:  gcc
 *
 *         Author:  Yu Yang
 *			Email:  yy1990cn@gmail.com
 * =======================================================================
 */

#ifndef _LOG_H_
#define _LOG_H_ 1

/* log macros */
#define LOG_FATAL(fmt, args...)   fprintf(stderr, "FATAL(file: %s, line: %d, func: %s):"fmt, __FILE__, __LINE__, __func__, ## args)
#define LOG_ERROR(fmt, args...)   fprintf(stderr, "ERROR(file: %s, line: %d, func: %s):"fmt, __FILE__, __LINE__, __func__, ## args)
#define LOG_WARN(fmt, args...)   fprintf(stderr, "WARN(file: %s, line: %d, func: %s):"fmt, __FILE__, __LINE__, __func__, ## args)
#define LOG_INFO(fmt, args...)   fprintf(stdout, "INFO(file: %s, line: %d, func: %s):"fmt, __FILE__, __LINE__, __func__, ## args)

#define LOG_ERR LOG_ERROR
/* this macro used to print message in debug mode */
#ifdef DEBUG
    #define LOG_DEBUG(fmt, args...)    fprintf(stdout, "DEBUG(file: %s, line: %d, func: %s):"fmt, __FILE__, __LINE__, __func__, ## args)
#else
    #define LOG_DEBUG(fmt, args...)    /* do nothing in release build*/
#endif

#endif /* _LOG_H_ */
