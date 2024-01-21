#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  
   if (s == NULL) {
    return 0;
  }
  size_t n = 0;
  while(s[n] != '\0') {
    n++;
  }
  return n;
}

char *strcpy(char *dst, const char *src) {
    char *original_dst = dst; // 保存目标字符串的起始地址

    // 将源字符串的每个字符复制到目标字符串中，直到遇到 '\0'
    while (*src != '\0') {
        *dst = *src;
        dst++;
        src++;
    }

    // 复制源字符串的结束符 '\0'
    *dst = '\0';

    return original_dst; // 返回目标字符串的起始地址
}

char *strncpy(char *dst, const char *src, size_t n) {
    char *original_dst = dst; // 保存目标字符串的起始地址

    // 将源字符串的每个字符复制到目标字符串中，直到达到最大长度 n 或者遇到 '\0'
    while (n > 0 && *src != '\0') {
        *dst = *src;
        dst++;
        src++;
        n--;
    }

    // 如果达到最大长度 n，但源字符串还没有结束，继续用 '\0' 填充
    while (n > 0) {
        *dst = '\0';
        dst++;
        n--;
    }

    return original_dst; // 返回目标字符串的起始地址
}

char *strcat(char *dst, const char *src) {
    char *original_dst = dst; // 保存目标字符串的起始地址

    // 移动到目标字符串的末尾
    while (*dst != '\0') {
        dst++;
    }

    // 将源字符串的每个字符追加到目标字符串的末尾
    while (*src != '\0') {
        *dst = *src;
        dst++;
        src++;
    }

    // 在连接后的字符串末尾添加 '\0'
    *dst = '\0';

    return original_dst; // 返回目标字符串的起始地址
}

// 比较两个字符串 s1 和 s2

int strcmp(const char *s1, const char *s2) {
   size_t i = 0;
  while(s1[i] != '\0' && s2[i] != '\0')
  {
	  if(s1[i] > s2[i])
		  return 1;
	  if(s1[i] < s2[i])
		  return -1;
	  i++;
  }
  if(s1[i] != '\0' && s2[i] == '\0')
	  return 1;
  if(s1[i] == '\0' && s2[i] != '\0')
	  return -1;
  return 0;
 
}

// 比较两个字符串 s1 和 s2 的前 n 个字符

int strncmp(const char *s1, const char *s2, size_t n) {
  while(n--)
	{
		if(*s1 > *s2)
			return 1;
		if(*s1 < *s2)
			return -1;
		s1++;
		s2++;
	}
	return 0;
}

// 将内存块 s 的前 n 个字节设置为值 c
void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;

    while (n > 0) {
        *p = (unsigned char)c;
        p++;
        n--;
    }

    return s;
}

// 将内存块 src 的前 n 个字节拷贝到 dst
void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;

    if (d < s || d >= s + n) {
        // 非重叠内存块，从前往后复制
        while (n > 0) {
            *d = *s;
            d++;
            s++;
            n--;
        }
    } else {
        // 重叠内存块，从后往前复制
        d += n;
        s += n;
        while (n > 0) {
            d--;
            s--;
            *d = *s;
            n--;
        }
    }

    return dst;
}

// 将内存块 in 的前 n 个字节拷贝到 out
void *memcpy(void *out, const void *in, size_t n) {
    unsigned char *o = out;
    const unsigned char *i = in;

    while (n > 0) {
        *o = *i;
        o++;
        i++;
        n--;
    }

    return out;
}

// 比较两个内存块 s1 和 s2 的前 n 个字节
int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;

    while (n > 0) {
      if(*p1 > *p2)
		  return 1;
	    if(*p1 < *p2)
		  return -1;


        p1++;
        p2++;
        n--;
    }

    return 0; // 前 n 个字节相等
}

#endif
