/* Stub for newlib libm: it expects __errno() to return a pointer to errno. */
static int _errno;

int *__errno(void) {
    return &_errno;
}
