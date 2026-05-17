#ifndef BLA_STUB_ARDUINO_H
#define BLA_STUB_ARDUINO_H

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#if defined(max)
#else
#define max(a,b) ((a)>(b)?(a):(b))
#endif

class Print
{
public:
    Print() = default;
    virtual ~Print() = default;
    virtual size_t write(uint8_t) = 0;

    size_t print(char c) {
        return write(static_cast<uint8_t>(c));
    }

    size_t print(const char s[]) {
        size_t n = 0;
        while (*s) { n += write(static_cast<uint8_t>(*s++)); }
        return n;
    }

    size_t print(int v) {
        return print(static_cast<long>(v));
    }

    size_t print(unsigned int v) {
        return print(static_cast<unsigned long>(v));
    }

    size_t print(long v) {
        if (v < 0) { print('-'); v = -v; }
        return print(static_cast<unsigned long>(v));
    }

    size_t print(unsigned long v) {
        char buf[12];
        int  pos = 0;
        do { buf[pos++] = '0' + (v % 10); v /= 10; } while (v);
        size_t n = 0;
        while (pos > 0) { n += write(static_cast<uint8_t>(buf[--pos])); }
        return n;
    }

    size_t print(double v) {
        long integer = static_cast<long>(v);
        size_t n = print(integer);
        n += write(static_cast<uint8_t>('.'));
        double frac = (v > 0) ? (v - integer) : (integer - v);
        for (int i = 0; i < 4; ++i) {
            frac *= 10;
            int digit = static_cast<int>(frac);
            n += write(static_cast<uint8_t>('0' + digit));
            frac -= digit;
        }
        return n;
    }

    size_t println() { return write(static_cast<uint8_t>('\n')); }
    size_t println(const char s[]) { size_t n = print(s); return n + write(static_cast<uint8_t>('\n')); }
    size_t println(double v)    { size_t n = print(v);    return n + write(static_cast<uint8_t>('\n')); }
    size_t println(int v)       { size_t n = print(v);    return n + write(static_cast<uint8_t>('\n')); }
};

#endif // BLA_STUB_ARDUINO_H
