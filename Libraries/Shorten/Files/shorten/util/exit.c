/******************************************************************************
*                                                                             *
*  Copyright (C) 1992-1995 Tony Robinson                                      *
*                                                                             *
*  See the file doc/LICENSE.shorten for conditions on distribution and usage  *
*                                                                             *
******************************************************************************/

/*
 * $Id: exit.c 19 2005-06-07 04:16:15Z vspader $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef MSDOS
#include <unistd.h>
#include <errno.h>
#endif
#include <setjmp.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "mkbshift.h"

extern char *argv0;
extern char *filenameo;
extern FILE *fileo;

#ifdef _WINDOWS
/* mrhmod - warn about attempt to use stderr (use perror_exit()/error_exit() instead) */
char *stderrWarningMsg = "caught attempt to use stderr";
#endif

jmp_buf exitenv;
char    *exitmessage;

/***************************************************************************/

void basic_exit(exitcode) int exitcode; {

  /* try to delete the output file on all abnormal exit conditions */
  if(exitcode != 0 && fileo != NULL && fileo != stdout) 
  {
    fclose(fileo);
    unlink(filenameo);
  }

  if(exitmessage == NULL)
    exit(exitcode < 0 ? 0 : exitcode);
  else
    longjmp(exitenv, exitcode);
}

/****************************************************************************
** error_exit() - standard error handler with printf() syntax
*/
# ifdef HAVE_STDARG_H
void error_exit(char* fmt, ...) {
  va_list args;

  va_start(args, fmt);
# else
void error_exit(va_alist) va_dcl {
  va_list args;
  char    *fmt;

  va_start(args);
  fmt = va_arg(args, char*);
# endif

  if(exitmessage == NULL)
  {
/*
#if defined(_WINDOWS) && defined(_DEBUG) && !defined(WIN32)
    _asm { int 3 }  / * mrhmod - catch if debugging * /
#endif
*/

#ifndef _WINDOWS  /* mrhmod - must use exitmessage 'cos stderr not available */
    fprintf(stderr, "%s: ", argv0);
    (void) vfprintf(stderr, fmt, args);
#endif /* _WINDOWS */
  }
  else
  {
    (void) vsprintf(exitmessage, fmt, args);
    strcat(exitmessage, "\n");
  }

  va_end(args);

  basic_exit(errno);
}

/****************************************************************************
** perror_exit() - system error handler with printf() syntax
**
** Appends system error message based on errno
*/
# ifdef HAVE_STDARG_H
void perror_exit(char* fmt, ...) {
  va_list args;

  va_start(args, fmt);
# else
void perror_exit(va_alist) va_dcl {
  va_list args;
  char    *fmt;

  va_start(args);
  fmt = va_arg(args, char*);
# endif

  if(exitmessage == NULL) {
/*
#if defined(_WINDOWS) && defined(_DEBUG) && !defined(WIN32)
    _asm { int 3 }  / * mrhmod - catch if debugging * /
#endif
*/

#ifndef _WINDOWS /* mrhmod - must use exitmessage 'cos stderr not available */
    fprintf(stderr, "%s: ", argv0);
    (void) vfprintf(stderr, fmt, args);
    (void) fprintf(stderr, ": ");
#ifndef MSDOS
    perror("\0");
#endif
    
#endif /* _WINDOWS */
  }
  else {
    (void) vsprintf(exitmessage, fmt, args);
    strcat(exitmessage, ": ");
    strcat(exitmessage,strerror(errno));
    strcat(exitmessage, "\n");
  }

  va_end(args);

  basic_exit(errno);
}

# ifdef HAVE_STDARG_H
void usage_exit(int exitcode, char* fmt, ...) {
  va_list args;

  va_start(args, fmt);
# else
void usage_exit(va_alist) va_dcl {
  va_list args;
  int     exitcode;
  char    *fmt;

  va_start(args);
  exitcode = va_arg(args, int);
  fmt      = va_arg(args, char*);
# endif

  if(exitmessage == NULL) {
#if defined(_WINDOWS) && defined(_DEBUG) && !defined(WIN32)
    _asm { int 3 }  /* mrhmod - catch if debugging */
#endif
    
#ifndef _WINDOWS  /* mrhmod - must use exitmessage 'cos stderr not available */
    if(fmt != NULL) {
      fprintf(stderr, "%s: ", argv0);
      (void) vfprintf(stderr, fmt, args);
    }
    fprintf(stderr, "%s: for more information use: %s -h\n", argv0, argv0);
#endif /* _WINDOWS */
  }
  else
  {
    (void) vsprintf(exitmessage, fmt, args);
    strcat(exitmessage, "\n");
  }

  va_end(args);

  basic_exit(exitcode);
} 	


# ifdef HAVE_STDARG_H
void update_exit(int exitcode, char* fmt, ...) {
  va_list args;

  va_start(args, fmt);
# else
void update_exit(va_alist) va_dcl {
  va_list args;
  int     exitcode;
  char    *fmt;

  va_start(args);
  exitcode = va_arg(args, int);
  fmt      = va_arg(args, char*);
# endif

  if(exitmessage == NULL) {
#if defined(_WINDOWS) && defined(_DEBUG) && !defined(WIN32)
    _asm { int 3 }  /* mrhmod - catch if debugging */
#endif

#ifndef _WINDOWS  /* mrhmod - must use exitmessage 'cos stderr not available */
    if(fmt != NULL) {
      fprintf(stderr, "%s: ", argv0);
      (void) vfprintf(stderr, fmt, args);
    }
    fprintf(stderr, "%s: version %s\n",argv0,VERSION);
    fprintf(stderr, "%s: please report this problem to ajr@softsound.com\n", argv0);
#endif /* _WINDOWS */
  }
#ifdef _WINDOWS /* mrhmod - output something */
  error_exit( fmt, args );
#endif

  va_end(args);

  basic_exit(exitcode);
}
