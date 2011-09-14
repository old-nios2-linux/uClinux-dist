#ifndef __DEBUG_H_
#define __DEBUG_H_
#define DEBUG
#ifdef DEBUG
void coreb_msg(char *fmt, ...);
#else
# define coreb_msg(fmt, ...) do {} while (0)
#endif
#endif
