/* UTF-8 safe truncation utility */

int utf8_safe_truncate(const char* str, int max_bytes) {
    if (!str || max_bytes <= 0) return 0;
    int len = 0;
    while (str[len]) len++;
    if (len <= max_bytes) return len;

    int pos = max_bytes;
    while (pos > 0 && (str[pos] & 0xC0) == 0x80) {
        pos--;
    }

    /* Check if the character at pos is complete within max_bytes */
    unsigned char c = (unsigned char)str[pos];
    int char_len = 0;
    if ((c & 0x80) == 0) char_len = 1;
    else if ((c & 0xE0) == 0xC0) char_len = 2;
    else if ((c & 0xF0) == 0xE0) char_len = 3;
    else if ((c & 0xF8) == 0xF0) char_len = 4;

    if (pos + char_len <= max_bytes) return max_bytes;
    return pos;
}
