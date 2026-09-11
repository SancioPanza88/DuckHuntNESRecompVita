/*
 * vita_compat.c — compat POSIX mancante su newlib/VitaSDK (solo __VITA__).
 *
 * - readlink(): runner/src/config.c la usa nel ramo Linux/Unix per risolvere
 *   la exe dir via /proc/self/exe. Su Vita /proc non esiste e newlib non
 *   fornisce readlink -> undefined reference in link. Ritorniamo -1 così
 *   nesrecomp_exe_dir() usa il fallback "./" (che dopo lo chdir di vita_main
 *   punta a ux0:data/DUCKHUNT1/, scrivibile).
 */
#ifdef __VITA__

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

ssize_t readlink(const char *path, char *buf, size_t bufsiz) {
    (void)path; (void)buf; (void)bufsiz;
    errno = ENOSYS;
    return -1;
}

#endif /* __VITA__ */
