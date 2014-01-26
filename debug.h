#ifndef __dbg_h__
#define __dbg_h__

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) { struct timeval tv; gettimeofday(&tv, 0); fprintf(stderr, "\033[1;37mDEBUG\033[0m [\033[0;34m%1d.%d\033[0m] \033[0;33m%s\033[0m:\033[0;33m%d\033[0m: " M "\n", (int)tv.tv_sec, (int)tv.tv_usec, __FILE__, __LINE__, ##__VA_ARGS__); }
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...) fprintf(stderr, "[\033[1;31mERROR\033[0m] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...) fprintf(stderr, "[\033[1;33mWARN\033[0m] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...) fprintf(stderr, "[\033[1;37mINFO\033[0m] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define sentinel(M, ...)  { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define check_mem(A) check((A), "Out of memory.")

#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__); errno=0; goto error; }

#define die(A, M, ...) { log_err(M, ##__VA_ARGS__); exit(A); }

#endif
