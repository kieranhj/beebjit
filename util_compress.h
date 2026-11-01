#ifndef BEEBJIT_UTIL_COMPRESS_H
#define BEEBJIT_UTIL_COMPRESS_H

int util_gunzip(size_t* p_dst_len,
                uint8_t* p_src,
                size_t src_len,
                uint8_t* p_dst);

int util_uncompress(size_t* p_dst_len,
                    uint8_t* p_src,
                    size_t src_len,
                    uint8_t* p_dst);

/* zlib-format deflate. Returns 0 on success; *p_dst_len is the space on
 * entry and the compressed size on exit.
 */
int util_compress(size_t* p_dst_len,
                  uint8_t* p_src,
                  size_t src_len,
                  uint8_t* p_dst);

#endif /* BEEBJIT_UTIL_COMPRESS_H */
