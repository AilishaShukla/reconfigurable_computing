// Small stand-in for Xilinx ap_int.h so kernels can be tested on a Mac.
// It only supports what the lab kernels use: wide ints, bit slices x(hi, lo),
// .range(hi, lo), and casts to and from int.
// It is NOT the real header. Real HLS errors only show up on the VM.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>

template <int W> class ap_int;

template <int W> class ap_range_ref;

template <int W> class ap_int {
public:
    static const int NW = (W + 63) / 64;
    uint64_t w[NW];

    ap_int() { clear(); }
    ap_int(long long v) { set_from_signed(v); }
    ap_int(int v) { set_from_signed(v); }
    ap_int(unsigned v) { set_from_signed((long long)v); }
    ap_int(long v) { set_from_signed(v); }
    ap_int(unsigned long long v) { set_from_signed((long long)v); }

    template <int W2> ap_int(const ap_int<W2> &o) {
        long long top = 0;
        clear();
        for (int i = 0; i < W && i < W2; i++) set_bit(i, o.get_bit(i));
        // sign extend
        if (W > W2 && o.get_bit(W2 - 1)) {
            top = 1;
            for (int i = W2; i < W; i++) set_bit(i, top);
        }
    }
    template <int W2> ap_int(const ap_range_ref<W2> &r);

    bool get_bit(int i) const { return (w[i / 64] >> (i % 64)) & 1ULL; }
    void set_bit(int i, bool b) {
        if (b) w[i / 64] |= (1ULL << (i % 64));
        else   w[i / 64] &= ~(1ULL << (i % 64));
    }

    // signed value of the low 64 bits (sign extended from bit W-1 when W < 64)
    long long to_int64() const {
        uint64_t v = w[0];
        if (W < 64) {
            uint64_t mask = (1ULL << W) - 1;
            v &= mask;
            if ((v >> (W - 1)) & 1ULL) v |= ~mask;
        }
        return (long long)v;
    }
    operator long long() const { return to_int64(); }

    ap_range_ref<W> operator()(int hi, int lo) { return ap_range_ref<W>(this, hi, lo); }
    ap_range_ref<W> range(int hi, int lo) { return ap_range_ref<W>(this, hi, lo); }
    ap_range_ref<W> operator()(int hi, int lo) const { return ap_range_ref<W>(const_cast<ap_int *>(this), hi, lo); }
    ap_range_ref<W> range(int hi, int lo) const { return ap_range_ref<W>(const_cast<ap_int *>(this), hi, lo); }

private:
    void clear() { for (int i = 0; i < NW; i++) w[i] = 0; }
    void set_from_signed(long long v) {
        uint64_t fill = v < 0 ? ~0ULL : 0ULL;
        for (int i = 0; i < NW; i++) w[i] = fill;
        w[0] = (uint64_t)v;
        trim();
    }
    void trim() {
        if (W % 64) w[NW - 1] &= (1ULL << (W % 64)) - 1;
    }
};

template <int W> class ap_range_ref {
public:
    ap_int<W> *p;
    int hi, lo;
    ap_range_ref(ap_int<W> *p_, int hi_, int lo_) : p(p_), hi(hi_), lo(lo_) {
        if (lo < 0 || hi >= W || lo > hi) {
            fprintf(stderr,
                    "\n*** BIT SLICE OUT OF RANGE: x(%d, %d) on a %d-bit value ***\n"
                    "    Check your (hi, lo) math. Valid bits are 0 to %d.\n",
                    hi, lo, W, W - 1);
            exit(2);
        }
    }
    int width() const { return hi - lo + 1; }

    // read: bits are returned as an unsigned value, like the real ap_range_ref
    unsigned long long to_uint64() const {
        unsigned long long v = 0;
        for (int i = 0; i < width() && i < 64; i++)
            if (p->get_bit(lo + i)) v |= (1ULL << i);
        return v;
    }
    operator unsigned long long() const { return to_uint64(); }

    // write
    ap_range_ref &operator=(long long v) {
        for (int i = 0; i < width(); i++) {
            bool b = i < 64 ? ((unsigned long long)v >> i) & 1ULL : (v < 0);
            p->set_bit(lo + i, b);
        }
        return *this;
    }
    ap_range_ref &operator=(int v) { return *this = (long long)v; }
    template <int W2> ap_range_ref &operator=(const ap_int<W2> &o) {
        for (int i = 0; i < width(); i++) {
            bool b = i < W2 ? o.get_bit(i) : o.get_bit(W2 - 1);
            p->set_bit(lo + i, b);
        }
        return *this;
    }
    ap_range_ref &operator=(const ap_range_ref &o) {
        ap_int<64> t((long long)o.to_uint64());
        return *this = t;
    }
};

template <int W>
template <int W2>
ap_int<W>::ap_int(const ap_range_ref<W2> &r) {
    clear();
    for (int i = 0; i < W && i < r.width(); i++) set_bit(i, r.p->get_bit(r.lo + i));
    // a slice is unsigned, so no sign extension; the top bit of ap_int<W> gives the sign
}

template <int W> using ap_uint = ap_int<W>;
