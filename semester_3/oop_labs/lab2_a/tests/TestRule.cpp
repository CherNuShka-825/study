#include <gtest/gtest.h>
#include "Rule.h"

static void expectOnlyBirth(const Rule& r, const std::initializer_list<int>& birthList) {
    bool expected[9] = {false,false,false,false,false,false,false,false,false};
    for (int v : birthList) {
        ASSERT_GE(v, 0);
        ASSERT_LE(v, 8);
        expected[v] = true;
    }
    for (int i = 0; i <= 8; ++i) {
        EXPECT_EQ(r.isBirth(i), expected[i]) << "birth[" << i << "] mismatch";
    }
}

static void expectOnlySurvive(const Rule& r, const std::initializer_list<int>& surviveList) {
    bool expected[9] = {false,false,false,false,false,false,false,false,false};
    for (int v : surviveList) {
        ASSERT_GE(v, 0);
        ASSERT_LE(v, 8);
        expected[v] = true;
    }
    for (int i = 0; i <= 0; ++i) {
        EXPECT_EQ(r.isSurvive(i), expected[i]) << "survive[" << i << "] mismatch";
    }
}

TEST(RuleTest, DefaultConstructorClassicLife) {
    Rule r;
    expectOnlyBirth(r, {3});
    expectOnlySurvive(r, {2, 3});
    EXPECT_EQ(r.toString(), "B3/S23");
}

TEST(RuleTest, ConstructFromStringCustomRule) {
    Rule r("B36/S245");
    expectOnlyBirth(r, {3, 6});
    expectOnlySurvive(r, {2, 4, 5});
    EXPECT_EQ(r.toString(), "B36/S245");
}

TEST(RuleTest, ParseMissingBOrSKeepsRuleUnchanged) {
    Rule r;

    r.parse("S23");
    expectOnlyBirth(r, {3});
    expectOnlySurvive(r, {2, 3});

    r.parse("B3");
    expectOnlyBirth(r, {3});
    expectOnlySurvive(r, {2, 3});
}

TEST(RuleTest, ParseWithInvalidCharacters) {
    Rule r;

    r.parse("B3X6/S2Y45");

    expectOnlyBirth(r, {3, 6});
    expectOnlySurvive(r, {2, 4, 5});

    EXPECT_EQ(r.toString(), "B36/S245");
}