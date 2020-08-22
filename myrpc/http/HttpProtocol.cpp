/* 用于修改请求和响应的头部.
 * 发送请求头部.
 * 接受请求行，头部和实体以及接受请求和响应.
 *  */ 

#include "HttpProtocol.h"
#include "HttpMsg.h"
#include "../network/SocketStreamBase.h"
#include <cstring>

namespace {

//用于分割字符串
char *SeparateStr(char **s, const char *del) {
    char *d, *tok;

    if (!s || !*s)
        return nullptr;

    tok = *s;
    d = strstr(tok, del);

    if (d) {
        *s = d + strlen(del);
        *d = '\0';
    }
    else 
        *s = nullptr;

    return tok;
}

//编码URL
void URLEncode(const char *source, char *dest, size_t length) {
    const char urlencstring[] = "0123456789abcedf";

    const char *p = source;
    char *q = dest;
    size_t n = 0;

    for ( ; *p && n < length; p++, q++, n++) {
        if (isalnum((int) *p)) {
            *q = *p;
        }
        else if (*p == ' ') 
            *q = '+';
        else {
            if (n > length - 3) {
                q++;
                break;
            }

            *q++ = '%';
            int digit = *p >> 4;
            *q++ = urlencstring[digit];
            digit = *p & 0xf;
            *q = urlencstring[digit];
            n += 2;
        }
    }

    *q = 0;
}

}

