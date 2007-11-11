#ifndef PERL_H
#define PERL_H

/**
 * not quite perl chomp, this function
 * hacks the newline off the end of
 * string str.
 */
char *chomp( char *str );

/**
 * trims the white space from the right 
 * of a string.
 */
char *rtrim(char *str);

/**
 * trims the white space from the left 
 * of a string.
 */
char *ltrim(char *str);

/**
 * trims the white space from the left
 * and the right sides of a string.
 */ 
char * trim(char *str);

/**
 * local library function prototyped here 
 * for split.
 */
int word_count( char pattern, char *s );

/**
 * split string *s on pattern pattern pointer
 * n_words holds the size of **
 */
char **split( char pattern, char *s, int *n_words );

/**
 * free memory allocated by split
 */
void split_free( char **split, int length ); 

#endif
