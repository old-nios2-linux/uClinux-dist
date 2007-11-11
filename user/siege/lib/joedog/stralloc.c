#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/ 

#ifdef  HAVE_STRING_H
# include <string.h>
#endif/*HAVE_STRING_H*/

#ifdef  HAVE_STRINGS_H
# include <strings.h>
#endif/*HAVE_STRINGS_H*/

#include <stdlib.h>
#include <error.h>

char *stralloc(char *str)
{ 
  char *retval;
 
  retval=calloc(strlen(str)+1, 1);
  if(!retval) {
    joe_fatal("Fatal memory allocation error");
  }
  strcpy(retval, str);
  return retval;
} 

