#pragma once
#include <bits/stdc++.h>

void print8(unsigned char x)
{
    for (int i = 8; i--;)
        putchar('0' ^ x >> i & 1);
}

struct BitVector
{
    int n; // the length of the BitVector
    int m; // how many unsigned char the bitvetor takes
    unsigned char *bitvector;

    // constructors and destructor
    BitVector(int n) : n(n), m(n + 7 >> 3)
    {
        // printf("A new BitVector: n = %d, m = %d\n", n, m);
        bitvector = (unsigned char *)malloc(m);
        memset(bitvector, 0, m);
    }
    BitVector(int n, uint64_t x) : n(n), m(n + 7 >> 3)
    {
        // printf("A new BitVector: n = %d, m = %d\n", n, m);
        assert(n <= 64);
        bitvector = (unsigned char *)malloc(m);
        memset(bitvector, 0, n);
        x &= (1ull << n) - 1;
        for (int i = 0; i < m && x; ++i, x >>= 8)
            bitvector[i] = x & (1 << 8) - 1;
    }
    BitVector(const BitVector &bv) : n(bv.n), m(bv.m), bitvector((unsigned char *)malloc(bv.m))
    {
        memcpy(bitvector, bv.bitvector, m);
    }
    BitVector &operator=(uint64_t x)
    {
        assert(n <= 64);
        x &= (1ull << n) - 1;
        for (int i = 0; i < m && x; ++i, x >>= 8)
            bitvector[i] = x & (1 << 8) - 1;
        return *this;
    }
    ~BitVector()
    {
        free(bitvector);
    }

    // operators
    bool operator==(uint64_t x) const
    {
        if (x >> n)
            return false;
        for (int i = 0; i < m; ++i)
        {
            if ((x & 255) != bitvector[i])
                return false;
            x >>= 8;
        }
        return true;
    }
    bool operator[](const int i) const
    {
        if (i < 0 || i >= n) printf("try to access %d in a bitset of length %d\n", i, n);

        assert(i >= 0 && i < n);
        return bitvector[i >> 3] >> (i & 7) & 1;
    }

    // slice of [l, r]
    uint64_t get(int l, int r)
    {
        // printf("get(%d, %d)\n", l, r);

        assert(r - l + 1 <= 64);
        assert(r < n);

        int bl = l >> 3, br = r >> 3;
        int il = l & 7, ir = r & 7;

        if (bl == br)
        {
            // printf("bitvector[%d] = ", bl);
            // print8(bitvector[bl]);
            // puts("");
            // printf("answer = %d\n", int((bitvector[bl] & (1 << ir + 1) - 1) >> il));

            return (bitvector[bl] & (1 << ir + 1) - 1) >> il;
        }

        uint64_t ans = il ? (bitvector[bl] & ~((1 << il) - 1)) >> il : bitvector[bl] & 255;
        int x = 8 - il;

        // printf("Take the first element, ans = %" PRIx64 "\n", ans);

        for (int i = bl + 1; i < br; ++i)
        {
            ans |= bitvector[i] << x;
            x += 8;
        }
        ans |= (bitvector[br] & (1 << ir + 1) - 1) << x;
        return ans;
    }
    void set(int l, int r, uint64_t val)
    {
        // printf("set(%d, %d, %" PRIu64 ")\n", l, r, val);

        assert(r - l + 1 <= 64);
        assert(r < n);

        int bl = l >> 3, br = r >> 3;
        int il = l & 7, ir = r & 7;

        if (bl == br)
        {
            // printf("bitvector[%d]: ", bl);
            // print8(bitvector[bl]);
            // printf(" -> ");
            // print8(bitvector[bl] & (1 << il) - 1);
            // printf(" | ");
            // print8(bitvector[bl] & ~((1 << ir + 1) - 1));
            // printf(" | ");
            // print8((val & (1 << r - l + 1) - 1) << il);

            bitvector[bl] = bitvector[bl] & (1 << il) - 1 | bitvector[bl] & ~((1 << ir + 1) - 1) | (val & (1 << r - l + 1) - 1) << il;

            // printf(" = ");
            // print8(bitvector[bl]);
            // puts("");
        }
        else
        {
            int x = 8 - il;
            bitvector[bl] = (il ? bitvector[bl] & (1 << il) - 1 : 0) | (val & (1 << x) - 1) << il;
            uint64_t ans = 0;
            for (int i = bl + 1; i < br; ++i)
            {
                bitvector[i] = val >> x & 255;
                x += 8;
            }
            bitvector[br] = bitvector[br] & ~((1 << ir + 1) - 1) | val >> x & (1 << ir + 1) - 1;
        }
    }
    void set(int i, bool val)
    {
        assert(i >= 0 && i < n);
        if (val)
            bitvector[i >> 3] |= 1 << (i & 7);
        else
            bitvector[i >> 3] &= ~(1 << (i & 7));
    }
    void print()
    {
        for (int j = n & 7 ? n & 7 : 8; j--;)
            printf("%d", int(bitvector[m - 1] >> j & 1));
        for (int i = m - 1; i--;)
            print8(bitvector[i]);
    }
};