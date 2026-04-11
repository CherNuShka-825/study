#ifndef LAB1_A_BITARRAY_H
#define LAB1_A_BITARRAY_H


#include <vector>
#include <string>


class BitArray {
private:
    using BlockT = unsigned long;
    static constexpr int bitsPerBlock = int(sizeof(BlockT) * 8);
    int sizeBits = 0;
    std::vector<BlockT> blocks;
    static int checkSize(int n);
    void checkIndex(int i) const;
    static std::size_t neededBlocks(int numBits);
    void maskTail();
    void requireSame(const BitArray &other) const;

public:
    BitArray();
    ~BitArray();

    explicit BitArray(int num_bits, unsigned long value = 0);
    BitArray(const BitArray& b);

    void swap(BitArray& b);
    BitArray& operator=(const BitArray& b);

    void resize(int num_bits, bool value = false);
    void clear();
    void push_back(bool bit);

    BitArray& operator&=(const BitArray& b);
    BitArray& operator|=(const BitArray& b);
    BitArray& operator^=(const BitArray& b);

    BitArray& operator<<=(int n);
    BitArray& operator>>=(int n);
    BitArray operator<<(int n) const;
    BitArray operator>>(int n) const;

    BitArray& set(int n, bool val = true);
    BitArray& set();
    BitArray& reset(int n);
    BitArray& reset();

    bool any() const;
    bool none() const;
    BitArray operator~() const;
    int count() const;

    bool operator[](int i) const;
    int size() const;
    bool empty() const;
    std::string to_string() const;

    friend bool operator==(const BitArray& a, const BitArray& b);
    friend bool operator!=(const BitArray& a, const BitArray& b);
    friend BitArray operator&(const BitArray& a, const BitArray& b);
    friend BitArray operator|(const BitArray& a, const BitArray& b);
    friend BitArray operator^(const BitArray& a, const BitArray& b);
};


#endif
