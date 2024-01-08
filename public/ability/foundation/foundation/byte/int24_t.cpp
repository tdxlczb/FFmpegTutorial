#include "foundation/byte/int24_t.h"

// constructors
int24_t::int24_t() : int24_t(0)
{
}

int24_t::int24_t(int value)
{
    _bytes[0] = ((uint8_t*) &value)[0];
    _bytes[1] = ((uint8_t*) &value)[1];
    _bytes[2] = ((uint8_t*) &value)[2];
}

int24_t::int24_t(const int24_t& orig)
{
    _bytes[0] = orig._bytes[0];
    _bytes[1] = orig._bytes[1];
    _bytes[2] = orig._bytes[2];
}

// cast-operators
int24_t::operator bool() const
{
    return ((int) *this);
}

int24_t::operator int() const
{
    return (_bytes[2] & 0x80 ? (0xFF << 24) : 0) | (_bytes[2] << 16) | (_bytes[1] << 8) | _bytes[0];
}

int24_t::operator double() const
{
    return (double) ((int) *this);
}

// alignment operators
int24_t& int24_t::operator=(int value)
{
    _bytes[0] = ((uint8_t*) &value)[0];
    _bytes[1] = ((uint8_t*) &value)[1];
    _bytes[2] = ((uint8_t*) &value)[2];

    return *this;
}

int24_t& int24_t::operator=(const int24_t& value)
{
    _bytes[0] = value._bytes[0];
    _bytes[1] = value._bytes[1];
    _bytes[2] = value._bytes[2];

    return *this;
}

// addition operators
int24_t int24_t::operator+() const
{
    return +((int) *this);
}

int24_t int24_t::operator+(int rvalue) const
{
    return ((int) *this) + rvalue;
}

int24_t int24_t::operator+(const int24_t& rvalue) const
{
    return ((int) *this) + (int) rvalue;
}

// subtraction operators
int24_t int24_t::operator-() const
{
    return -((int) *this);
}

int24_t int24_t::operator-(int rvalue) const
{
    return ((int) *this) - rvalue;
}

int24_t int24_t::operator-(const int24_t& rvalue) const
{
    return ((int) *this) - ((int) rvalue);
}

// multiplication operators
int24_t int24_t::operator*(int rvalue) const
{
    return ((int) *this) * rvalue;
}

int24_t int24_t::operator*(const int24_t& rvalue) const
{
    return ((int) *this) * ((int) rvalue);
}

// division operators
int24_t int24_t::operator/(int rvalue) const
{
    return ((int) *this) / rvalue;
}

int24_t int24_t::operator/(const int24_t& rvalue) const
{
    return ((int) *this) / ((int) rvalue);
}

// mod-operators
int24_t int24_t::operator%(int rvalue) const
{
    return ((int) *this) % rvalue;
}

int24_t int24_t::operator%(const int24_t& rvalue) const
{
    return ((int) *this) % ((int) rvalue);
}

// increment, decrement
int24_t int24_t::operator++()
{
    this->operator+=(1);
    return *this;
}

int24_t int24_t::operator++(int)
{
    auto tmp = *this;
    this->operator+=(1);

    return tmp;
}

int24_t int24_t::operator--()
{
    this->operator-=(1);
    return *this;
}

int24_t int24_t::operator--(int)
{
    auto tmp = *this;
    this->operator-=(1);

    return tmp;
}

// extra-alignments
void int24_t::operator+=(int rvalue)
{
    this->operator=(this->operator+(rvalue));
}

void int24_t::operator+=(const int24_t& rvalue)
{
    this->operator=(this->operator+(rvalue));
}

void int24_t::operator-=(int rvalue)
{
    this->operator=(this->operator-(rvalue));
}

void int24_t::operator-=(const int24_t& rvalue)
{
    this->operator=(this->operator-(rvalue));
}

void int24_t::operator*=(int rvalue)
{
    this->operator=(this->operator*(rvalue));
}

void int24_t::operator*=(const int24_t& rvalue)
{
    this->operator=(this->operator*(rvalue));
}

void int24_t::operator/=(int rvalue)
{
    this->operator=(this->operator/(rvalue));
}

void int24_t::operator/=(const int24_t& rvalue)
{
    this->operator=(this->operator/(rvalue));
}

void int24_t::operator%=(int rvalue)
{
    this->operator=(this->operator/(rvalue));
}

void int24_t::operator%=(const int24_t& rvalue)
{
    this->operator=(this->operator%(rvalue));
}

// logic operators
bool int24_t::operator==(int rvalue) const
{
    return ((int) *this) == rvalue;
}

bool int24_t::operator==(const int24_t& rvalue) const
{
    return ((int) *this) == ((int) rvalue);
}

bool int24_t::operator!=(int rvalue) const
{
    return !this->operator==(rvalue);
}

bool int24_t::operator!=(const int24_t& rvalue) const
{
    return !this->operator==(rvalue);
}

bool int24_t::operator<(int rvalue) const
{
    return ((int) *this) < rvalue;
}

bool int24_t::operator<(const int24_t& rvalue) const
{
    return ((int) *this) < ((int) rvalue);
}

bool int24_t::operator<=(int rvalue) const
{
    return ((int) *this) <= rvalue;
}

bool int24_t::operator<=(const int24_t& rvalue) const
{
    return ((int) *this) <= ((int) rvalue);
}

bool int24_t::operator>(int rvalue) const
{
    return ((int) *this) > rvalue;
}

bool int24_t::operator>(const int24_t& rvalue) const
{
    return ((int) *this) > ((int) rvalue);
}

bool int24_t::operator>=(int rvalue) const
{
    return ((int) *this) >= rvalue;
}

bool int24_t::operator>=(const int24_t& rvalue) const
{
    return ((int) *this) > ((int) rvalue);
}

// output
std::ostream& operator<<(std::ostream& os, const int24_t& obj)
{
    return os << (int) obj;
}
