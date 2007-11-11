/**
 * Load Post Data
 *
 * Copyright (C) 2002-2006 by
 * Jeffrey Fulmer - <jeff@joedog.org>
 * This file is distributed as part of Siege
 *
 * Copyright (c) Ian F. Darwin 1986-1995.
 * Software written by Ian F. Darwin and others;
 * maintained 1995-present by Christos Zoulas and others.
 * (see: file-4.03 from ftp.astron.com for details)
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
#include <stdio.h> 
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> 
#include <load.h>
#include <errno.h>
#include <joedog/joedog.h>

typedef unsigned long unichar;

#define F 0   /* character never appears in text */
#define T 1   /* character appears in plain ASCII text */
#define I 2   /* character appears in ISO-8859 text */
#define X 3   /* character appears in non-ISO extended ASCII (Mac, IBM PC) */

static char text_chars[256] = 
{
  F, F, F, F, F, F, F, T, T, T, T, F, T, T, F, F,  /* 0x0X */
  F, F, F, F, F, F, F, F, F, F, F, T, F, F, F, F,  /* 0x1X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x2X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x3X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x4X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x5X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x6X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, F,  /* 0x7X */
  X, X, X, X, X, T, X, X, X, X, X, X, X, X, X, X,  /* 0x8X */
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,  /* 0x9X */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xaX */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xbX */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xcX */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xdX */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xeX */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I   /* 0xfX */
};

int
looks_ascii(const unsigned char *buf, size_t nbytes, unichar *ubuf, size_t *ulen)
{
  size_t  i;
  *ulen = 0;

  for (i = 0; i < nbytes; i++) {
    int t = text_chars[buf[i]];
    if (t != T)
      return 0;
    ubuf[(*ulen)++] = buf[i];
  }
  return 1;
}


int
looks_latin1(const unsigned char *buf, size_t nbytes, unichar *ubuf, size_t *ulen)
{
  size_t i;
  *ulen = 0;

  for(i = 0; i < nbytes; i++){
    int t = text_chars[buf[i]];
    if(t != T && t != I)
      return 0;

    ubuf[(*ulen)++] = buf[i];
  }
  return 1;
}

int
looks_utf8(const unsigned char *buf, size_t nbytes, unichar *ubuf, size_t *ulen)
{
  size_t i;
  int    n;
  unichar c;
  int gotone = 0;
  *ulen = 0;

  for(i = 0; i < nbytes; i++){
    if((buf[i] & 0x80) == 0){                 /* 0xxxxxxx is plain ASCII */
      if (text_chars[buf[i]] != T)
        return 0;
        ubuf[(*ulen)++] = buf[i];
      } else if ((buf[i] & 0x40) == 0) {      /* 10xxxxxx never 1st byte */
        return 0;
      } else {                                /* 11xxxxxx begins UTF-8 */
        int following;
        if((buf[i] & 0x20) == 0){             /* 110xxxxx */
          c = buf[i] & 0x1f;
          following = 1;
        } else if ((buf[i] & 0x10) == 0) {    /* 1110xxxx */
          c = buf[i] & 0x0f;
          following = 2;
       } else if ((buf[i] & 0x08) == 0) {      /* 11110xxx */
          c = buf[i] & 0x07;
          following = 3;
       } else if ((buf[i] & 0x04) == 0) {      /* 111110xx */
          c = buf[i] & 0x03;
          following = 4;
       } else if ((buf[i] & 0x02) == 0) {      /* 1111110x */
          c = buf[i] & 0x01;
          following = 5;
       } else
          return 0;

       for(n = 0; n < following; n++){
         i++;
         if(i >= nbytes)
           goto done;
         if((buf[i] & 0x80) == 0 || (buf[i] & 0x40))
           return 0;
         c = (c << 6) + (buf[i] & 0x3f);
       }
     ubuf[(*ulen)++] = c;
     gotone = 1;
   }
 }
done:
 return gotone;   /* don't claim it's UTF-8 if it's all 7-bit */
}


int
looks_extended(const unsigned char *buf, size_t nbytes, unichar *ubuf, size_t *ulen)
{
  size_t  i;
  *ulen = 0;

  for(i = 0; i < nbytes; i++){
    int t = text_chars[buf[i]];
    if(t != T && t != I && t != X)
      return 0;
    ubuf[(*ulen)++] = buf[i];
  }
  return 1;
}

int
looks_unicode(const unsigned char *buf, size_t nbytes, unichar *ubuf, size_t *ulen)
{
  int    bigend;
  size_t i;

  if(nbytes < 2)
    return 0;

  if(buf[0] == 0xff && buf[1] == 0xfe)
    bigend = 0;
  else if (buf[0] == 0xfe && buf[1] == 0xff)
    bigend = 1;
  else
    return 0;

  *ulen = 0;

  for(i = 2; i + 1 < nbytes; i += 2){
    if(bigend)
      ubuf[(*ulen)++] = buf[i + 1] + 256 * buf[i];
    else
      ubuf[(*ulen)++] = buf[i] + 256 * buf[i + 1];
    if(ubuf[*ulen - 1] == 0xfffe)
      return 0;
    if(ubuf[*ulen - 1] < 128 && text_chars[(size_t)ubuf[*ulen - 1]] != T)
      return 0;
  }
  return 1 + bigend;
}

/**
 * maps a file to our address space 
 * and returns it the calling function.
 */
void 
load_file(URL *U, char *file)
{
  FILE     *fp;
  size_t   len = 0;
  struct   stat st; 
  char     *filename;
  char     postdata[POST_BUF]; 
  unichar  ubuf[POST_BUF+1];   
  size_t   ulen;

  filename = trim(file);
  memset(postdata, 0, POST_BUF);

  if((lstat(filename, &st) == 0) || (errno != ENOENT)){ 
    len = st.st_size;  
    if((fp = fopen(filename, "r")) == NULL){
      joe_error("could not open file: %s", filename);
      return;
    }
    if((fread(postdata, 1, len, fp )) == len){
      if(looks_ascii(postdata,len,ubuf,&ulen))
        trim(postdata);
      else if(looks_utf8(postdata,len,ubuf,&ulen))
        trim(postdata);
    } else {
      joe_error( "unable to read file: %s", filename );
    }
    fclose(fp);
  }

  if(strlen(postdata) > 0){
    U->postlen  = strlen(postdata);
    U->postdata = malloc(U->postlen);
    memcpy(U->postdata, postdata, U->postlen);
    U->postdata[U->postlen] = 0;
  } 
  return;
}

#undef F
#undef T
#undef I
#undef X

