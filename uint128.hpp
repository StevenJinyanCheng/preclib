#ifndef PRECLIB_UINT128_HPP
#define PRECLIB_UINT128_HPP
#include<iostream>
#include<stdio.h>
#include<stdint.h>
#include<string>
struct preclib_uint128{
    uint64_t hi, lo;
    preclib_uint128():hi(0), lo(0) {}
    preclib_uint128(uint64_t high, uint64_t low): hi(high), lo(low) {}
    preclib_uint128(uint64_t a): hi(0), lo(a) {}
    preclib_uint128 operator+(const preclib_uint128& b) const {
        preclib_uint128 r;
        r.lo = lo + b.lo;
        r.hi = hi + b.hi + (r.lo < lo ? 1 : 0);
        return r;
    }
    preclib_uint128 operator-(const preclib_uint128& b) const {
        preclib_uint128 r;
        r.lo = lo - b.lo;
        r.hi = hi - b.hi - (lo < b.lo ? 1 : 0);
        return r;
    }
    preclib_uint128 operator*(const preclib_uint128& b) const {

        uint64_t a_lo = lo & 0xFFFFFFFF;
        uint64_t a_hi = lo >> 32;
        uint64_t b_lo = b.lo & 0xFFFFFFFF;
        uint64_t b_hi = b.lo >> 32;

        uint64_t p0 = a_lo * b_lo;
        uint64_t p1 = a_lo * b_hi;
        uint64_t p2 = a_hi * b_lo;
        uint64_t p3 = a_hi * b_hi;

        uint64_t mid1 = p1 + (p0 >> 32);
        uint64_t mid2 = p2 + (mid1 & 0xFFFFFFFF);

        preclib_uint128 r;
        r.lo = (mid2 << 32) | (p0 & 0xFFFFFFFF);
        r.hi = hi * b.lo + lo * b.hi + p3 + (mid1 >> 32) + (mid2 >> 32);
        return r;
    }

    bool operator<(const preclib_uint128& b) const {
        if (hi != b.hi) return hi < b.hi;
        return lo < b.lo;
    }
    bool operator>=(const preclib_uint128& b) const {
        return !(*this < b);
    }
    bool operator==(const preclib_uint128& b) const {
        return hi == b.hi && lo == b.lo;
    }
    bool operator!=(const preclib_uint128& b) const {
        return !(*this == b);
    }
    bool operator<=(const preclib_uint128& b) const {
        return (*this < b) || (*this == b);
    }
    bool operator>(const preclib_uint128& b) const {
        return !(*this <= b);
    }
    operator bool() const {
        return hi != 0 || lo != 0;
    }
    preclib_uint128 rsft32() const {
        return preclib_uint128(hi >> 32, (hi << 32) | (lo >> 32));
    }
    preclib_uint128 trsft32() {
        *this = this->rsft32();
        return *this;
    }
};
#endif // PRECLIB_UINT128_HPP