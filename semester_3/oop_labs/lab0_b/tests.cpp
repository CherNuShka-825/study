#include <gtest/gtest.h>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <cmath>
#include "CSV.h"

static bool feq(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) <= eps * std::max(1.0, std::max(std::fabs(a), std::fabs(b)));
}

struct Row { std::string w; unsigned long long c; double f; };

static std::vector<Row> run_fill_csv(const std::string& text) {
    std::istringstream in(text);
    std::ostringstream out;
    CSV::fill_csv(in, out);

    std::vector<Row> rows;
    std::istringstream parsed(out.str());
    std::string line;
    while (std::getline(parsed, line)) {
        if (line.empty()) continue;

        size_t p1 = line.find(';');
        EXPECT_NE(p1, std::string::npos) << "Bad CSV line (no first ';'): " << line;
        if (p1 == std::string::npos) continue;

        size_t p2 = line.find(';', p1 + 1);
        EXPECT_NE(p2, std::string::npos) << "Bad CSV line (no second ';'): " << line;
        if (p2 == std::string::npos) continue;

        Row r;
        r.w = line.substr(0, p1);
        r.c = std::stoull(line.substr(p1 + 1, p2 - p1 - 1));
        r.f = std::stod(line.substr(p2 + 1));
        rows.push_back(r);
    }
    return rows;
}


TEST(CSV_Private, FillMap_CapturesLastTokenAtLineEnd) {
    std::istringstream in("Hello, world! Hello\n123 apples\nend");
    std::map<std::string, unsigned long long> m;
    CSV::fill_map(m, in);

    EXPECT_EQ(m["Hello"],  2u);
    EXPECT_EQ(m["world"],  1u);
    EXPECT_EQ(m["123"],    1u);
    EXPECT_EQ(m["apples"], 1u);
    EXPECT_EQ(m["end"],    1u);
}

TEST(CSV_Private, FillMap_EmptyInputAndOnlyDelimiters) {
    {
        std::istringstream in("");
        std::map<std::string, unsigned long long> m;
        CSV::fill_map(m, in);
        EXPECT_TRUE(m.empty());
    }
    {
        std::istringstream in(" ,.;:!? \t\r\n --- ");
        std::map<std::string, unsigned long long> m;
        CSV::fill_map(m, in);
        EXPECT_TRUE(m.empty());
    }
}

TEST(CSV_Private, FillMap_CaseSensitivityAndDigits) {
    std::istringstream in("Cat CAT cAt 42 42");
    std::map<std::string, unsigned long long> m;
    CSV::fill_map(m, in);

    EXPECT_EQ(m["Cat"], 1u);
    EXPECT_EQ(m["CAT"], 1u);
    EXPECT_EQ(m["cAt"], 1u);
    EXPECT_EQ(m["42"],  2u);
}

TEST(CSV_Private, FillVector_AndCountingWords) {
    std::istringstream in("a a a\nb b\nc");

    std::vector<CSV::word_degree> vec;
    CSV::fill_vector(vec, in);

    unsigned long long sum = CSV::counting_words(vec);
    EXPECT_EQ(sum, 6u);

    std::map<std::string, unsigned long long> have;
    for (const auto& wd : vec) have[wd.word] = wd.count;
    EXPECT_EQ(have["a"], 3u);
    EXPECT_EQ(have["b"], 2u);
    EXPECT_EQ(have["c"], 1u);
}

TEST(CSV_Private, Comp_StrictGreater) {
    CSV::word_degree a{"apple", 5};
    CSV::word_degree b{"banana", 2};
    CSV::word_degree c{"citrus", 5};

    EXPECT_TRUE (CSV::comp(a, b));
    EXPECT_FALSE(CSV::comp(b, a));
    EXPECT_FALSE(CSV::comp(a, c)); // строго ">"
}

TEST(CSV_CSVOut, SortedByCountAndFreqSumIsOne) {
    auto rows = run_fill_csv("x x x\ny y\nz");
    ASSERT_EQ(rows.size(), 3u);

    EXPECT_EQ(rows[0].w, "x"); EXPECT_EQ(rows[0].c, 3u);
    EXPECT_EQ(rows[1].w, "y"); EXPECT_EQ(rows[1].c, 2u);
    EXPECT_EQ(rows[2].w, "z"); EXPECT_EQ(rows[2].c, 1u);

    double fsum = 0.0;
    for (const auto& r : rows) fsum += r.f;
    EXPECT_TRUE(feq(fsum, 1.0));

    EXPECT_TRUE(feq(rows[0].f, 3.0/6.0));
    EXPECT_TRUE(feq(rows[1].f, 2.0/6.0));
    EXPECT_TRUE(feq(rows[2].f, 1.0/6.0));
}
