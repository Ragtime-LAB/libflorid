#ifndef BLA_STUB_PRINTABLE_H
#define BLA_STUB_PRINTABLE_H

class Print;

class Printable
{
public:
    virtual ~Printable() = default;
    virtual size_t printTo(Print& p) const = 0;
};

#endif // BLA_STUB_PRINTABLE_H
