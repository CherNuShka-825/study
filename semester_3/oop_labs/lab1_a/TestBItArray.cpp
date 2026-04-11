#include <gtest/gtest.h>
#include <stdexcept>
#include "BitArray.h"


static BitArray fromeStringBuildBitArray(const std::string& s) {
    BitArray b((int)s.size());
    for (int pos = 0; pos < (int)s.size(); ++pos) {
        char c = s[pos];
        if (c != '0' && c != '1') {
            throw std::invalid_argument("from_string_msb: only '0'/'1' allowed");
        }
        if (c == '1') {
            int idx = (int)s.size() - 1 - pos;
            b.set(idx, true);
        }
    }
    return b;
}

TEST(BitArray_Basics, DefaultCtor_Empty) {
    BitArray b;
    EXPECT_EQ(b.size(), 0);
    EXPECT_TRUE(b.empty());
    EXPECT_TRUE(b.none());
    EXPECT_EQ(b.count(), 0);
    EXPECT_EQ(b.to_string(), "");
}

TEST(BitArray_Basics, CtorWithSizeAndValueLowBits) {
    BitArray b(8, 11UL);
    EXPECT_EQ(b.size(), 8);
    EXPECT_FALSE(b.empty());
    EXPECT_EQ(b[0], true);
    EXPECT_EQ(b[1], true);
    EXPECT_EQ(b[2], false);
    EXPECT_EQ(b[3], true);
    EXPECT_EQ(b.to_string(), "00001011");
    EXPECT_EQ(b.count(), 3);
}

TEST(BitArray_Access, OutOfRangeThrows) {
    BitArray b(5);
    EXPECT_THROW((void)b[5], std::out_of_range);
    EXPECT_THROW((void)b[-1], std::out_of_range);
    EXPECT_THROW(b.set(5), std::out_of_range);
    EXPECT_THROW(b.reset(6), std::out_of_range);
}

TEST(BitArray_String, ToStringOrder) {
    auto b = fromeStringBuildBitArray("101001");
    EXPECT_EQ(b.size(), 6);
    EXPECT_EQ(b.to_string(), "101001");
    EXPECT_TRUE(b[5]);
    EXPECT_FALSE(b[4]);
    EXPECT_TRUE(b[3]);
    EXPECT_FALSE(b[2]);
    EXPECT_FALSE(b[1]);
    EXPECT_TRUE(b[0]);
}

TEST(BitArray_SizeOps, ResizeGrowWithFillFalseTrue) {
    BitArray b(3);
    EXPECT_EQ(b.to_string(), "000");

    b.resize(7);
    EXPECT_EQ(b.size(), 7);
    EXPECT_EQ(b.to_string(), "0000000");
    EXPECT_EQ(b.count(), 0);

    b.set(0).set(6);
    b.resize(12, true);
    EXPECT_EQ(b.size(), 12);
    for (int i = 7; i <= 11; ++i) EXPECT_TRUE(b[i]);
    EXPECT_TRUE(b[0]);
    EXPECT_TRUE(b[6]);
}

TEST(BitArray_SizeOps, ResizeShrinkTruncates) {
    auto b = fromeStringBuildBitArray("111101");
    b.resize(3);
    EXPECT_EQ(b.size(), 3);
    EXPECT_EQ(b.to_string(), "101");
}

TEST(BitArray_PushBack, Basic) {
    BitArray b;
    b.push_back(1);
    b.push_back(0);
    b.push_back(1);
    EXPECT_EQ(b.size(), 3);
    EXPECT_EQ(b.to_string(), "101");
    EXPECT_EQ(b.count(), 2);
}

TEST(BitArray_BitOps, AndOrXor_EqualSizes) {
    auto a = fromeStringBuildBitArray("1011001");
    auto b = fromeStringBuildBitArray("0010111");

    BitArray c_and = a & b;
    BitArray c_or  = a | b;
    BitArray c_xor = a ^ b;

    EXPECT_EQ(c_and.to_string(), "0010001");
    EXPECT_EQ(c_or .to_string(), "1011111");
    EXPECT_EQ(c_xor.to_string(), "1001110");

    BitArray t = a;
    t &= b; EXPECT_EQ(t.to_string(), "0010001");
    t = a;  t |= b; EXPECT_EQ(t.to_string(), "1011111");
    t = a;  t ^= b; EXPECT_EQ(t.to_string(), "1001110");
}

TEST(BitArray_Compare, DifferentSizesNotEqual) {
    auto a = fromeStringBuildBitArray("10101");
    auto b = fromeStringBuildBitArray("010101");
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(BitArray_Core, ClearResetsToEmpty) {
    auto a = fromeStringBuildBitArray("101101");
    EXPECT_FALSE(a.empty());
    a.clear();
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(a.size(), 0);
    EXPECT_EQ(a.to_string(), "");
    EXPECT_EQ(a.count(), 0);
}

TEST(BitArray_CopyAssign, DeepCopyIndependence) {
    auto a = fromeStringBuildBitArray("101001");
    BitArray b = a;
    EXPECT_TRUE(a == b);
    b.set(0, false);
    EXPECT_NE(a[0], b[0]);
    BitArray c(1);
    c = a;
    EXPECT_TRUE(c == a);
    a.set(5, false);
    EXPECT_NE(c[5], a[5]);
}

TEST(BitArray_Swap, SwapsSizeAndData) {
    auto a = fromeStringBuildBitArray("1110001");
    auto b = fromeStringBuildBitArray("01");
    auto sa = a.to_string();
    auto sb = b.to_string();
    a.swap(b);
    EXPECT_EQ(a.to_string(), sb);
    EXPECT_EQ(b.to_string(), sa);
    EXPECT_EQ(a.size(), 2);
    EXPECT_EQ(b.size(), 7);
}

TEST(BitArray_Count, ManyBitsPatternEvery3rd) {
    const int BPB = int(sizeof(unsigned long) * 8);
    const int N = 3 * BPB + 7;
    BitArray a(N);
    int expected = 0;
    for (int i = 0; i < N; ++i) {
        if (i % 3 == 0) { a.set(i); ++expected; }
    }
    EXPECT_EQ(a.count(), expected);
    EXPECT_TRUE(a.any());
    EXPECT_FALSE(a.none());
}

TEST(BitArray_MaskTail, SetAllMasksTailProperly) {
    const int BPB = int(sizeof(unsigned long) * 8);
    const int N = BPB + 5;
    BitArray a(N);
    a.set();
    EXPECT_EQ(a.count(), N);

    auto r = a >> 1;
    EXPECT_EQ(r.count(), N - 1);
    auto l = a << 1;
    EXPECT_LE(l.count(), N - 1);
}

TEST(BitArray_Shifts, BigShiftsAndZeroFill) {
    const int BPB = int(sizeof(unsigned long) * 8);
    BitArray a(2 * BPB + 3);
    a.set(0).set(BPB).set(2 * BPB + 2);
    auto l = a << (BPB + 1);
    EXPECT_TRUE(l[BPB + 1]);
    EXPECT_TRUE(l[2 * BPB + 1]);
    auto r = a >> BPB;
    EXPECT_TRUE(r[0]);
    EXPECT_TRUE(r[BPB + 2]);
}

TEST(BitArray_Inversion, DoubleNotIsIdentity) {
    auto a = fromeStringBuildBitArray("1001010110001");
    auto b = ~~a;
    EXPECT_EQ(b.size(), a.size());
    EXPECT_EQ(b.to_string(), a.to_string());
    EXPECT_TRUE(a == b);
}
