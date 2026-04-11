#ifndef LAB0_B_CSV_H
#define LAB0_B_CSV_H

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>

#ifdef CSV_ENABLE_TESTS
    #include <gtest/gtest_prod.h>
#endif

class CSV {
private:
    struct word_degree {
        std::string word;
        unsigned long long count;
    };
    static void fill_vector(std::vector<word_degree>& vector_words, std::istream& file_in);
    static bool comp(const word_degree& a, const word_degree& b);
    static void fill_map(std::map<std::string, unsigned long long>& words, std::istream& file_in);
    static unsigned long long counting_words(std::vector<word_degree>& vector_words);

#ifdef CSV_ENABLE_TESTS
    FRIEND_TEST(CSV_Private, FillMap_CapturesLastTokenAtLineEnd);
    FRIEND_TEST(CSV_Private, FillMap_EmptyInputAndOnlyDelimiters);
    FRIEND_TEST(CSV_Private, FillMap_CaseSensitivityAndDigits);
    FRIEND_TEST(CSV_Private, FillVector_AndCountingWords);
    FRIEND_TEST(CSV_Private, Comp_StrictGreater);
#endif

public:
    static void fill_csv(std::istream& file_in, std::ostream& file_out);
};


#endif
