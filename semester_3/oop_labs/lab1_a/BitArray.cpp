#include "BitArray.h"
#include <stdexcept>
#include <algorithm>


// Validates that bit size is non-negative; throws if invalid.
int BitArray::checkSize(int n) {
    if (n < 0) throw std::invalid_argument("size < 0");
    return n;
}

// Checks that index is within [0, sizeBits); throws if out of range.
void BitArray::checkIndex(int i) const {
    if (i < 0 || i >= sizeBits) throw std::out_of_range("wrong index");
}

// Calculates how many storage blocks are required for numBits bits.
std::size_t BitArray::neededBlocks(int numBits) {
    return numBits == 0 ? 0 : (std::size_t(numBits) + bitsPerBlock - 1) / bitsPerBlock;
}

// Clears unused bits in the last block beyond logical sizeBits.
void BitArray::maskTail() {
    if (blocks.empty()) return;
    int used = sizeBits % bitsPerBlock;
    if (used == 0) return;
    BlockT mask = (BlockT(1) << used) - 1;
    blocks.back() &= mask;
}

// Ensures that both arrays have equal size; throws if they differ.
void BitArray::requireSame(const BitArray &other) const {
    if (sizeBits != other.sizeBits) throw std::invalid_argument("sizes differ");
}

BitArray::BitArray() = default; // Default constructor: creates an empty bit array.
BitArray::~BitArray() = default; // Default destructor.

// Constructs bit array of given size and optional initial value (LSB-first).
BitArray::BitArray(int num_bits, unsigned long value)
    :sizeBits(checkSize(num_bits)),
     blocks(neededBlocks(num_bits), 0) {
    if (sizeBits > 0) {
        int k = std::min(bitsPerBlock, sizeBits);
        BlockT mask = (k == bitsPerBlock) ? ~BlockT(0) : ((BlockT(1) << k) - 1);
        blocks[0] = BlockT(value) & mask;
        maskTail();
    }
}

// Copy constructor (default-generated).
BitArray::BitArray(const BitArray& b) = default;

// Exchanges internal data with another BitArray.
void BitArray::swap(BitArray &b) {
    std::swap(sizeBits, b.sizeBits);
    blocks.swap(b.blocks);
}

// Copy assignment using copy-and-swap idiom.
BitArray& BitArray::operator=(const BitArray& b) {
    if (this != &b) {
        BitArray tmp(b);
        swap(tmp);
    }
    return *this;
}

// Resizes bit array to num_bits; fills new bits with given value when growing.
void BitArray::resize(int num_bits, bool value) {
    int n = checkSize(num_bits);
    std::size_t oldBlocks = blocks.size();
    std::size_t newBlocks = neededBlocks(n);

    if (newBlocks != oldBlocks) blocks.resize(newBlocks, 0);
    if (n > sizeBits) {
        int start = sizeBits;
        int end = n;
        if (start % bitsPerBlock != 0 && !blocks.empty()) {
            int b = start / bitsPerBlock;
            int off = start % bitsPerBlock;
            int minToFill1Block = std::min(bitsPerBlock - off, end - start);
            BlockT mask = (((BlockT(1) << minToFill1Block) - 1) << off);
            if (value) blocks[b] |= mask; else blocks[b] &= ~mask;
            start += minToFill1Block;
        }
        while (start + bitsPerBlock <= end) {
            int b = start / bitsPerBlock;
            blocks[b] = value ? ~BlockT(0) : BlockT(0);
            start += bitsPerBlock;
        }
        if (start < end) {
            int b = start / bitsPerBlock;
            int chunk = end - start;
            BlockT mask = (BlockT(1) << chunk) - 1;
            if (value) blocks[b] |= mask; else blocks[b] &= ~mask;
        }
    }
    sizeBits = n;
    maskTail();
}

// Clears all bits and releases storage.
void BitArray::clear() {
    blocks.clear();
    sizeBits = 0;
}

// Appends one bit to the end (as new MSB).
void BitArray::push_back(bool bit) {
    resize(sizeBits + 1, false);
    set(sizeBits - 1, bit);
}

// Bitwise AND-assigns this array with another (same size required).
BitArray& BitArray::operator&=(const BitArray& b) {
    requireSame(b);
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        blocks[i] &= b.blocks[i];
    }
    maskTail();
    return *this;
}

// Bitwise OR-assigns this array with another (same size required).
BitArray& BitArray::operator|=(const BitArray& b) {
    requireSame(b);
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        blocks[i] |= b.blocks[i];
    }
    maskTail();
    return *this;
}

// Bitwise XOR-assigns this array with another (same size required).
BitArray& BitArray::operator^=(const BitArray &b) {
    requireSame(b);
    for(std::size_t i = 0; i < blocks.size(); ++i) {
        blocks[i] ^=b.blocks[i];
    }
    maskTail();
    return *this;
}

// Logical left shift by n bits (zero-fill).
// Negative n delegates to right shift.
BitArray& BitArray::operator<<=(int n) {
    if (n < 0) return (*this) >>= (-n);
    if (n == 0 || sizeBits == 0) return *this;
    if (n >= sizeBits) {
        std::fill(blocks.begin(), blocks.end(), 0);
        return *this;
    }

    int bs = n / bitsPerBlock;
    int rs = n % bitsPerBlock;

    if (bs > 0) {
        for (int i = int(blocks.size()) - 1; i >= 0; --i)
            blocks[i] = (i - bs >= 0) ? blocks[i - bs] : 0;
    }

    if (rs != 0) {
        for (int i = int(blocks.size()) - 1; i >= 0; --i) {
            BlockT low = (i - 1 >= 0) ? blocks[i - 1] : BlockT(0);
            blocks[i] = BlockT(blocks[i] << rs) | BlockT(low >> (bitsPerBlock - rs));
        }
    }
    maskTail();
    return *this;
}

// Logical right shift by n bits (zero-fill).
// Negative n delegates to left shift.
BitArray& BitArray::operator>>=(int n) {
    if (n < 0) return (*this) <<= (-n);
    if (n == 0 || sizeBits == 0) return *this;
    if (n >= sizeBits) {
        std::fill(blocks.begin(), blocks.end(), 0);
        return *this;
    }

    const int bs = n / bitsPerBlock;
    const int rs = n % bitsPerBlock;

    if (bs > 0) {
        for (std::size_t i = 0; i < blocks.size(); ++i)
            blocks[i] = (i + bs < blocks.size()) ? blocks[i + bs] : BlockT(0);
    }

    if (rs != 0) {
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            BlockT hi = (i + 1 < blocks.size()) ? blocks[i + 1] : BlockT(0);
            blocks[i] = BlockT(blocks[i] >> rs) |
                        BlockT(hi << (bitsPerBlock - rs));
        }
    }

    maskTail();
    return *this;
}

// Returns a left-shifted copy of this array.
BitArray BitArray::operator<<(int n) const {
    BitArray tmp(*this);
    tmp <<= n;
    return tmp;
}

// Returns a right-shifted copy of this array.
BitArray BitArray::operator>>(int n) const {
    BitArray tmp(*this);
    tmp >>= n;
    return tmp;
}

// Sets bit at index n to val (true=1, false=0).
BitArray& BitArray::set(int n, bool val) {
    checkIndex(n);
    std::size_t b = std::size_t(n) / bitsPerBlock;
    int ind = n % bitsPerBlock;
    BlockT mask = BlockT(1) << ind;
    blocks[b] = val ? (blocks[b] | mask) : (blocks[b] & ~mask);
    return *this;
}

// Sets all bits to 1 (within logical size).
BitArray& BitArray::set() {
    if (sizeBits == 0) return *this;
    std::fill(blocks.begin(), blocks.end(), ~BlockT(0));
    maskTail();
    return *this;
}

// Resets (clears) bit at index n.
BitArray& BitArray::reset(int n) {
    return set(n, false);
}

// Resets all bits to 0.
BitArray& BitArray::reset() {
    std::fill(blocks.begin(), blocks.end(), BlockT(0));
    return *this;
}

// Returns true if any bit is set to 1.
bool BitArray::any() const {
    for (BlockT b : blocks) {
        if (b) return true;
    }
    return false;
}

// Returns true if all bits are 0.
bool BitArray::none() const {
    return !any();
}

// Returns bitwise inversion of this bit array (within logical size).
BitArray BitArray::operator~() const {
    BitArray rev(*this);
    for (auto& b: rev.blocks) {
        b = ~b;
    }
    rev.maskTail();
    return rev;
}

// Counts and returns the total number of bits set to 1.
int BitArray::count() const {
    int c = 0;
    for (BlockT b : blocks) {
        while (b) {
            b &= (b - 1);
            ++c;
        }
    }
    return c;
}

// Returns value of bit at index i (true = 1, false = 0).
bool BitArray::operator[](int i) const {
    checkIndex(i);
    std::size_t blockIndex = std::size_t(i) / bitsPerBlock;
    int offset = i % bitsPerBlock;
    return ((blocks[blockIndex] >> offset) & BlockT(1)) != 0;
}

// Returns current number of bits in the array.
int BitArray::size() const {
    return sizeBits;
}

// Returns true if the array contains no bits (size == 0).
bool BitArray::empty() const {
    return sizeBits == 0;
}

// Converts the bit array to string representation (MSB→LSB order).
std::string BitArray::to_string() const {
    std::string s;
    s.reserve(sizeBits);
    for (int i = sizeBits - 1; i >= 0; --i) {
        s.push_back((*this)[i] ? '1' : '0');
    }
    return s;
}

// Compares two bit arrays for equality (same size and identical bits).
bool operator==(const BitArray& a, const BitArray& b) {
    if (a.size() != b.size()) return false;
    for (int i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// Returns true if arrays differ in size or bit pattern.
bool operator!=(const BitArray& a, const BitArray& b) {
    return !(a == b);
}

// Returns bitwise AND of two equal-sized arrays.
// @throws std::invalid_argument if sizes differ.
BitArray operator&(const BitArray& b1, const BitArray& b2) {
    if (b1.size() != b2.size()) {
        throw std::invalid_argument("sizes differ");
    }
    BitArray tmp(b1);
    tmp &= b2;
    return tmp;
}

// Returns bitwise OR of two equal-sized arrays.
// @throws std::invalid_argument if sizes differ.
BitArray operator|(const BitArray& b1, const BitArray& b2) {
    if (b1.size() != b2.size()) {
        throw std::invalid_argument("sizes differ");
    }
    BitArray tmp(b1);
    tmp |= b2;
    return tmp;
}

// Returns bitwise XOR of two equal-sized arrays.
// @throws std::invalid_argument if sizes differ.
BitArray operator^(const BitArray& b1, const BitArray& b2) {
    if (b1.size() != b2.size()) {
        throw std::invalid_argument("sizes differ");
    }
    BitArray tmp(b1);
    tmp ^= b2;
    return tmp;
}
