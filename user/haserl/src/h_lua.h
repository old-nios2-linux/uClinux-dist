#ifndef H_LUA_H
#define H_LUA_H	1


void bash_open(char *shell);
void lua_destroy(void);
void lua_exec(buffer_t *buf, char *str);
void lua_echo(buffer_t *buf, char *str, size_t len);
void lua_eval(buffer_t *buf, char *str, size_t len);
void lua_setup(char *shell, list_t *env);
void lua_doscript(buffer_t *script, char *name);



#endif
