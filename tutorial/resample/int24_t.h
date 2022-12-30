#ifndef INT24_T_H
#define INT24_T_H


#include <stdint.h>
#include <ostream>


#define INT24_MIN -8388608
#define INT24_MAX 8388607


class int24_t {
public:
    int24_t();
    int24_t(int value);
    int24_t(const int24_t& orig);

    operator bool() const;
    operator int() const;
    operator double() const;

    int24_t& operator =(int value);
    int24_t& operator =(const int24_t& value);

    int24_t operator +() const;
    int24_t operator +(int rvalue) const;
    int24_t operator +(const int24_t& rvalue) const;
    int24_t operator -() const;
    int24_t operator -(int rvalue) const;
    int24_t operator -(const int24_t& rvalue) const;
    int24_t operator *(int rvalue) const;
    int24_t operator *(const int24_t& rvalue) const;
    int24_t operator /(int rvalue) const;
    int24_t operator /(const int24_t& rvalue) const;
    int24_t operator %(int rvalue) const;
    int24_t operator %(const int24_t& rvalue) const;

    int24_t operator ++();
    int24_t operator ++(int not_used);
    int24_t operator --();
    int24_t operator --(int not_used);

    void operator +=(int rvalue);
    void operator +=(const int24_t& rvalue);
    void operator -=(int rvalue);
    void operator -=(const int24_t& rvalue);
    void operator *=(int value);
    void operator *=(const int24_t& rvalue);
    void operator /=(int rvalue);
    void operator /=(const int24_t& rvalue);
    void operator %=(int rvalue);
    void operator %=(const int24_t& rvalue);

    bool operator ==(int rvalue) const;
    bool operator ==(const int24_t& rvalue) const;
    bool operator !=(int rvalue) const;
    bool operator !=(const int24_t& rvalue) const;
    bool operator <(int rvlaue) const;
    bool operator <(const int24_t& rvalue) const;
    bool operator <=(int rvalue) const;
    bool operator <=(const int24_t& rvalue) const;
    bool operator >(int rvalue) const;
    bool operator >(const int24_t& rvalue) const;
    bool operator >=(int rvalue) const;
    bool operator >=(const int24_t& rvalue) const;

    friend std::ostream& operator <<(std::ostream& os, const int24_t& obj);


private:
    uint8_t _bytes[3];
};

#endif // INT24_T_H
