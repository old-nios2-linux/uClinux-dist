/**
 * Cookies Support
 *
 * Copyright (C) 2000, 2001, 2002 by
 * Jeffrey Fulmer - <jdfulmer@armstrong.com>
 * Copyright (C) 2002 the University of Kansas
 * This file is distributed as part of Siege 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * --
 *
 */ 
#include <setup.h>
#include <time.h>
#include <cookie.h>
#include <joedog/joedog.h>
#include <joedog/boolean.h>

typedef struct 
{
  char*  name;
  char*  value;
  char*  domain;
  char*  path;
  time_t expires;
  int    secure;
} PARSED_COOKIE;

void
free_cookie(PARSED_COOKIE* ck)
{
  xfree(ck->name);
  xfree(ck->value);
  xfree(ck->domain);
  xfree(ck->path);
}

COOKIE *cookie;

/**  
 * This function takes a string in the format
 * "Mon, 01-Jan-96 13:45:35 GMT" or "Mon,  1 Jan 1996 13:45:35 GMT" 
 * or "dd-mm-yyyy" and returns a conversion in clock format or 0.
 */ 
static time_t 
mktime2(const char *string, int absolute) 
{
  const char *s;
  time_t now, clock2;
  int day     = 0;
  int month   = 0;
  int year    = 0; 
  int hour    = 0; 
  int minutes = 0; 
  int seconds = 0;
  const char *start;
  char temp[8];

  /*
   *  Make sure we have a string to parse. 
   */
  if(!(string && *string))
    return(0);
  s = string;

  /*
   *  Skip any lead alphabetic "Day, " field and
   *  seek a numeric day field. 
   */
  while(*s != '\0' && !isdigit((unsigned char)*s))
    s++;
  if(*s == '\0')
    return(0);

  /*
   *  Get the numeric day and convert to an integer.
   */
  start = s;
  while( *s != '\0' && isdigit((unsigned char)*s))
    s++;
  if(*s == '\0' || (s - start) > 2){
    return(0);
  }
  strncpy(temp, start, (int)(s - start));
  if(temp != NULL)
    day = atoi(temp);
  if(day < 1 || day > 31)
    return(0);
  /*
   *  Get the month string and convert to an integer.
   */
  while(*s != '\0' && !isalnum((unsigned char)*s))
    s++;
  if(*s == '\0')
    return(0);
  start = s;
  while( *s != '\0' && isalnum((unsigned char)*s))
    s++;
  if(( *s == '\0' ) || 
    (s - start) < (isdigit((unsigned char)*(s - 1)) ? 2 : 3) ||
    (s - start) > (isdigit((unsigned char)*(s - 1)) ? 2 : 9))
    return(0);
  temp[2] = temp[3] = '\0';
  strncpy(temp, start, (isdigit((unsigned char)*(s - 1)) ? 2 : 3));
  switch( toupper(temp[0])){
    case '0':
    case '1':
      if(temp != NULL)
        month = atoi(temp);
      if( month < 1 || month > 12 )
        return(0);
      break;
    case 'A':
      if(!strcasecmp(temp, "Apr"))
        month = 4;
      else if(!strcasecmp(temp, "Aug"))
        month = 8;
      else
        return(0);
      break;
    case 'D':
      if(!strcasecmp(temp, "Dec"))
        month = 12;
      else
        return(0);
      break;
    case 'F':
      if(!strcasecmp(temp, "Feb")) 
        month = 2;
      else
        return(0);
      break;
    case 'J':
      if(!strcasecmp(temp, "Jan")) 
        month = 1;
      else if(!strcasecmp(temp, "Jun")) 
        month = 6;
      else if (!strcasecmp(temp, "Jul")) 
        month = 7;
      else 
        return(0);
      break;
    case 'M':
      if(!strcasecmp(temp, "Mar")) 
        month = 3;
      else if(!strcasecmp(temp, "May")) 
        month = 5;
      else 
        return(0);
      break;
    case 'N':
      if(!strcasecmp(temp, "Nov"))
        month = 11;
      else 
        return(0);
      break;
    case 'O':
      if(!strcasecmp(temp, "Oct")) 
        month = 10;
      else 
        return(0);
      break;
    case 'S':
      if(!strcasecmp(temp, "Sep"))
        month = 9;
      else 
        return(0);
      break;
    default:
      return(0);
  }

  /*
   *  Get the numeric year string and convert to an integer.
   */
  while(*s != '\0' && !isdigit((unsigned char)*s))
    s++;
  if(*s == '\0')
    return(0);
  start = s;
  while(*s != '\0' && isdigit((unsigned char)*s))
    s++;
  if((s - start) == 4){
    strncpy(temp, start, 4);
  } else if((s - start) == 2){
    now = time(NULL);
    /*
     * Assume that received 2-digit dates >= 70 are 19xx; others
     * are 20xx.  Only matters when dealing with broken software
     * (HTTP server or web page) which is not Y2K compliant.  The
     * line is drawn on a best-guess basis; it is impossible for
     * this to be completely accurate because it depends on what
     * the broken sender software intends.  (This totally breaks
     * in 2100 -- setting up the next crisis...) - BL
     */
    if(start != NULL && atoi(start) >= 70)
     strncpy(temp, "19", 2);
    else
      strncpy(temp, "20", 2);
    strncat(temp, start, 2);
    temp[4] = '\0';
  } else {
    return 0;
  }
  if(temp != NULL)
    year = atoi(temp);

  /*
   *  Get the numeric hour string and convert to an integer.
   */
  while(*s != '\0' && !isdigit((unsigned char)*s))
    s++;
  if(*s == '\0'){
    hour    = 0;
    minutes = 0;
    seconds = 0;
  } else {
    start = s;
    while(*s != '\0' && isdigit((unsigned char)*s))
      s++;
    if(*s != ':' || (s - start) > 2)
      return(0);
    strncpy(temp, start, (int)(s - start));
    if(temp != NULL)
      hour = atoi(temp);

    /*
     *  Get the numeric minutes string and convert to an integer.
     */
    while(*s != '\0' && !isdigit(( unsigned char)*s))
      s++;
    if(*s == '\0')
      return(0);
    start = s;
    while(*s != '\0' && isdigit(( unsigned char)*s))
      s++;
    if(*s != ':' || (s - start) > 2)
      return(0);
    strncpy(temp, start, (int)(s - start));
    if(temp != NULL)
      minutes = atoi(temp);

    /*
     *  Get the numeric seconds string and convert to an integer.
     */
    while(*s != '\0' && !isdigit(( unsigned char)*s))
      s++;
    if(*s == '\0')
      return(0);
    start = s;
    while(*s != '\0' && isdigit(( unsigned char)*s))
      s++;
    /* We don't care if we're at the end of the string here.
       Theoretically there should be a timezone specification
       but we don't care that much if it's not there. */
    if((s - start) > 2)
      return(0);
    strncpy(temp, start, (int)(s - start));
    if(temp != NULL)
      seconds = atoi(temp);
  }

  /*
   *  Convert to clock format (seconds since 00:00:00 Jan 1 1970),
   *  but then zero it if it's in the past and "absolute" is not
   *  TRUE. 
   */
  month -= 3;
  if(month < 0){
    month += 12;
    year--;
  }
  day += (year - 1968) * 1461 / 4;
  day += ((((month * 153) + 2) / 5) - 672);
  clock2 = (time_t)((day * 60 * 60 * 24) + (hour * 60 * 60) + (minutes * 60) + seconds );
  if(!absolute && (long)(time((time_t *)0) - clock2) >= 0)
    clock2 = (time_t)0;
  return(clock2);
}

void
parse_cookie(char *cookiestr, PARSED_COOKIE* ck)
{
  char *lval, *rval;

  if (cookiestr==NULL || ck==NULL) return;

  ck->name = ck->value = ck->domain = ck->path = NULL;
  ck->expires = ck->secure = 0;

  lval = cookiestr;

  while(*cookiestr && *cookiestr != '=')
    cookiestr++;

  if(!*cookiestr) return;

  *cookiestr++ = 0; /* NULL-terminate lval (replace '=') and position at start of rval */

  rval = cookiestr;
  while(*cookiestr && *cookiestr != ';')
    cookiestr++;

  if(!*cookiestr) return;

  *cookiestr++ = 0;

  ck->name  = (lval != NULL) ? xstrdup(lval) : NULL;
  ck->value = (rval != NULL) ? xstrdup(rval) : NULL; 
  /* get the biggest possible positive value */
  ck->expires = 0;
  ck->expires = ~ck->expires;
  if(ck->expires < 0){
    ck->expires = ~(1 << ((sizeof(ck->expires) * 8) - 1));
  }
  if(ck->expires < 0){
    ck->expires = (ck->expires >> 1) * -1;
  }

  while( *cookiestr ) {

    while( isspace( (unsigned char)*cookiestr ))
      cookiestr++;

    if (!*cookiestr) break;

    lval = cookiestr;
    while( *cookiestr && *cookiestr != '=' )
      cookiestr++;

    if ( !strcasecmp (lval, "secure")) {
      ck->secure = 1;
      rval = NULL;
    }
    else {
      if ( !*cookiestr) return;

      *cookiestr++ = 0;

      rval = cookiestr;
      while( *cookiestr && *cookiestr != ';' )
        cookiestr++;
      *cookiestr++ = 0;
    }

    if ( !strcasecmp (lval, "domain")) {
      /*printf ("[%d] parse_cookie() - lval='%s' rval='%s'\n", pthread_self(), lval, rval ? rval : "NULL");*/
      ck->domain = (rval != NULL) ? xstrdup( rval ) : NULL; 
    }
    else
    if ( !strcasecmp (lval, "expires")) {
      ck->expires = mktime2 (rval, TRUE);
      /*printf ("[%d] parse_cookie() - expires lval='%s' rval='%s' %lu\n", pthread_self(), lval, rval ? rval : "NULL", ck->expires);*/
    }
    else
    if ( !strcasecmp (lval, "path")) {
      ck->path = (rval != NULL) ? xstrdup( rval ) : NULL; 
    }
  }

  /*
  printf ("parse_cookie() - name='%s' value='%s' domain='%s' path='%s' expires=%d secure=%d\n",
          ck->name ? ck->name : "NULL",
          ck->value ? ck->value : "NULL",
          ck->domain ? ck->domain : "NULL",
          ck->path ? ck->path : "NULL",
          ck->expires, ck->secure);
  */
}

/**
 * insert values into list
 */
int
add_cookie(pthread_t id, char *host, char *cookiestr)
{
  char *name, *value;
  int  found = FALSE;
  CNODE *cur, *pre, *fresh = NULL; 
  PARSED_COOKIE ck;

  parse_cookie(cookiestr, &ck);
  name = ck.name;
  value = ck.value;
  if(( name == NULL || value == NULL )) return -1;

  pthread_mutex_lock(&(cookie->mutex)); 
   
  for( cur=pre=cookie->first; cur != NULL; pre=cur, cur=cur->next ){
    if(( cur->threadID == id )&&( !strcasecmp(cur->name, name))){
      xfree( cur->name );
      xfree( cur->value );
      xfree( cur->domain ); 
      cur->name  = xstrdup( name );
      cur->value = xstrdup( value );
      cur->expires = ck.expires;
      if( !ck.domain )
        cur->domain = xstrdup( host );
      else
        cur->domain = xstrdup( ck.domain ); 
      found = TRUE;
      break;
    }
  }
  if( !found ){
    fresh = (CNODE*)xmalloc(sizeof(CNODE));
    if(!fresh) joe_fatal("out of memory!"); 
    fresh->threadID = id;
    fresh->name     = xstrdup( name );
    fresh->value    = xstrdup( value );
    fresh->expires  = ck.expires; 
    if( !ck.domain )
      fresh->domain = xstrdup( host );
    else
      fresh->domain = xstrdup( ck.domain );
    fresh->next = cur;
    if( cur==cookie->first )
      cookie->first = fresh;
    else
      pre->next = fresh;    
  }
  if( name  != NULL ) xfree( name );
  if( value != NULL ) xfree( value );

  pthread_mutex_unlock(&(cookie->mutex));

  return 0;
}

int
delete_cookie( pthread_t id, char *name )
{
  CNODE  *cur, *pre;
  
  pthread_mutex_lock(&(cookie->mutex));
  for( cur=pre=cookie->first; cur != NULL; pre=cur, cur=cur->next ){
    if( cur->threadID == id ){
      if( !strcasecmp( cur->name, name )){
        pre->next = cur->next;
        /* ksjuse: XXX: this breaks when the cookie to remove comes first */
        xfree(cur);
        if( my.debug ){printf("Cookie deleted: %ld => %s\n",(long)id,name); fflush(stdout);}
        break;
      }
    } 
    else{
      continue; 
    }
  }
  pthread_mutex_unlock(&(cookie->mutex));
  return 0;
}


/* 
  Delete all cookies associated with the given id.
  return 0 (as delete_cookie does)?
*/
int
delete_all_cookies(pthread_t id)
{
  CNODE  *cur, *pre;
  
  pthread_mutex_lock(&(cookie->mutex));
  for( pre=NULL, cur=cookie->first; cur != NULL; pre=cur, cur=cur->next ){
    if( cur->threadID == id ){
      if( my.debug ){printf("Cookie deleted: %ld => %s\n",(long)id,cur->name); fflush(stdout);}
      /* delete this cookie */
      if( cur == cookie->first ){
        /* deleting the first */
        cookie->first = cur->next;
        pre = cookie->first;
      } else {
        /* deleting inner */
        pre->next = cur->next;
      }
      xfree(cur->name);
      xfree(cur->value);
      xfree(cur->domain); 
      xfree(cur);
      cur = pre;
      /* cur is NULL now when we deleted the last cookie;
       * cur was cookie->first and first->next was NULL.
       * check that before incremeting (cur=cur->next).
       */
      if (cur == NULL)
  break;
    } 
  }
  pthread_mutex_unlock(&(cookie->mutex));
  return 0;
}

/**
 * get_cookie returns a char * with a compete value of the Cookie: header
 * It does NOT return the actual Cookie struct.      
 */
char *
get_cookie_header(pthread_t id, char *host, char *newton)
{
  int    dlen, hlen; 
  CNODE  *pre, *cur;
  time_t now;
  char   oreo[MAX_COOKIE_SIZE];  

  memset(oreo, '\0', sizeof oreo);
  hlen = strlen(host); 

  pthread_mutex_lock(&(cookie->mutex));
  now = time(NULL);

  for(cur=pre=cookie->first; cur != NULL; pre=cur, cur=cur->next){
    dlen = cur->domain ? strlen( cur->domain ) : 0;

    if(cur->threadID == id){
      if(!strcasecmp(cur->domain, host)){
        if(cur->expires <= now){
          continue;
        }
        if(strlen(oreo) > 0)
          strncat(oreo, ";",      sizeof oreo);
        strncat(oreo, cur->name,  sizeof oreo);
        strncat(oreo, "=",        sizeof oreo);
        strncat(oreo, cur->value, strlen(cur->value));
      }
      if((dlen < hlen) && (!strcasecmp(host + (hlen - dlen), cur->domain))){
        if (cur->expires <= now) {
          continue;
        }
        if(strlen(oreo) > 0)
          strncat(oreo, ";",      sizeof oreo);
        strncat(oreo, cur->name,  sizeof oreo);
        strncat(oreo, "=",        sizeof oreo);
        strncat(oreo, cur->value, strlen(cur->value));
      }
    }
  }
  if(strlen(oreo) > 0){
    strncpy(newton, "Cookie: ", 8);
    strncat(newton, oreo, sizeof(oreo));
    strncat(newton, "\015\012", 2);
  }
  pthread_mutex_unlock(&(cookie->mutex));

  return newton;
}

/*
  Helper util, displays the contents of Cookie
*/
void
display_cookies()
{
  CNODE *cur;
 
  pthread_mutex_lock(&(cookie->mutex));
 
  printf ("Linked list contains:\n");
  for( cur=cookie->first; cur != NULL; cur=cur->next ) {
    printf ("Index: %ld\tName: %s Value: %s\n", (long)cur->threadID, cur->name, cur->value);
  }
 
  pthread_mutex_unlock(&(cookie->mutex));
 
  return;
}

