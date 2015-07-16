#ifndef S2N_BASE64_H
#define S2N_BASE64_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
static inline size_t base64len(size_t n) {
    return (n + 2) / 3 * 4;
}
static size_t base64(char *buf, size_t nbuf, const unsigned char *p, size_t n) {
    const char t[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t i, m = base64len(n);
    unsigned x;
    if (nbuf >= m)
    for (i = 0; i < n; ++i) {
        x = p[i] << 0x10;
        x |= (++i < n ? p[i] : 0) << 0x08;
        x |= (++i < n ? p[i] : 0) << 0x00;
        *buf++ = t[x >> 3 * 6 & 0x3f];
        *buf++ = t[x >> 2 * 6 & 0x3f];
        *buf++ = (((n - 0 - i) >> 31) & '=') |
        (~((n - 0 - i) >> 31) & t[x >> 1 * 6 & 0x3f]);
        *buf++ = (((n - 1 - i) >> 31) & '=') |
        (~((n - 1 - i) >> 31) & t[x >> 0 * 6 & 0x3f]);
    }
    return m;
}
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* S2N_BASE64_H */
