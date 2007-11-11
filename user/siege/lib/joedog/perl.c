#ifdef HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <perl.h>
#include <ctype.h>

#define SPLITSZ 256

/**
 * not quite perl chomp, this function
 * hacks the newline off the end of a 
 * string.
 */
char *
chomp( char *str )
{
  if( *str && str[strlen(str)-1]=='\n' ) str[strlen(str)-1] = 0;
  return str;
}

/**
 * rtrim
 */
char *
rtrim(char *str)
{
  char *ptr;
  int   len;
 
  len = strlen( str );
  for( ptr = str + len - 1; ptr >= str && isspace((int)*ptr ); --ptr );
  
  ptr[1] = '\0';
 
  return str;
}

/**
 * ltrim: trim white space off left of str 
 */ 
char *
ltrim(char *str)
{
  char *ptr;
  int  len;
 
  for( ptr = str; *ptr && isspace ((int)*ptr); ++ptr );
 
  len = strlen( ptr );
  memmove( str, ptr, len + 1 );
 
  return str;
}

/**
 * trim: calls ltrim and rtrim
 */ 
char *
trim(char *str)
{
  char *ptr;
  ptr = rtrim( str );
  str = ltrim( ptr );
  return str;
} 

int
word_count( char pattern, char *s )
{
  int in_word_flag = 0;
  int count = 0;
  char *ptr;
 
  ptr = s;
  while( *ptr ){
    if(( *ptr ) != pattern ){
      if( in_word_flag == 0 )
        count++;
      in_word_flag = 1;
    }
    else
      in_word_flag = 0;
    ptr++;
  }
  return count;
} 

char **
split( char pattern, char *s, int *n_words )
{
  char **words;
  char *str0, *str1;
  int i;
  
  *n_words = word_count(pattern, s);
  if( *n_words == 0 )
    return NULL;
 
  words = xmalloc( *n_words * sizeof (*words));
  if( !words )
    return NULL;

  str0 = s;
  i = 0;
  while( *str0 ){
    size_t len;
    str1 = strchr( str0, pattern );
    if( str1 != NULL )
      len = str1 - str0;
    else
      len = strlen( str0 );

    /**
     * if len is 0 then str0 and str1 match
     * which means the string begins with a
     * separator. we don't want to allocate 
     * memory for an empty string, we just want 
     * to increment the pointer. on 0 we decrement 
     * i since it will be incremented below...
     */
    if( len == 0 ) i--; 
    else{
      words[i] = (char*)xmalloc(SPLITSZ);
      memset(words[i], 0, SPLITSZ ); 
      memcpy(words[i], (char*)str0, SPLITSZ); 
      words[i][len] = '\0';
    }

    if( str1 != NULL )
      str0 = ++str1;
    else
      break;
    i++;
  }
  return words;
} 

void
split_free( char **split, int length )
{
  int x;
  for( x = 0; x < length; x ++ )
    if( split[x] != NULL ){
      char *tmp = split[x];
      free( tmp );
    }
  free( split );
 
  return;
} 
