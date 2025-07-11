#include "strutils.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

// Tokenize Tests
TEST(TokenizeTest, Basic) {
    std::string input = "hello world test string";
    std::vector<std::string> tokens = tokenize(input, " ", false);
    
    EXPECT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "test");
    EXPECT_EQ(tokens[3], "string");
}

TEST(TokenizeTest, MultipleDelimiters) {
    std::string input = "hello,world;test:string";
    std::vector<std::string> tokens = tokenize(input, ",;:", false);
    
    EXPECT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "test");
    EXPECT_EQ(tokens[3], "string");
}

TEST(TokenizeTest, EmptyTokens) {
    std::string input = "hello,,world,,test";
    
    // Without empty tokens
    std::vector<std::string> tokensWithoutEmpty = tokenize(input, ",", false);
    EXPECT_EQ(tokensWithoutEmpty.size(), 3);
    EXPECT_EQ(tokensWithoutEmpty[0], "hello");
    EXPECT_EQ(tokensWithoutEmpty[1], "world");
    EXPECT_EQ(tokensWithoutEmpty[2], "test");
    
    // With empty tokens
    std::vector<std::string> tokensWithEmpty = tokenize(input, ",", true);
    EXPECT_EQ(tokensWithEmpty.size(), 5);
    EXPECT_EQ(tokensWithEmpty[0], "hello");
    EXPECT_EQ(tokensWithEmpty[1], "");
    EXPECT_EQ(tokensWithEmpty[2], "world");
    EXPECT_EQ(tokensWithEmpty[3], "");
    EXPECT_EQ(tokensWithEmpty[4], "test");
}

TEST(TokenizeTest, EmptyInput) {
    std::string input = "";
    std::vector<std::string> tokens = tokenize(input, ",", false);
    
    EXPECT_TRUE(tokens.empty());
}

// EndsWith Tests
TEST(EndsWithTest, Basic) {
    EXPECT_TRUE(endswith("hello world", "world"));
    EXPECT_FALSE(endswith("hello world", "hello"));
    EXPECT_TRUE(endswith("test", "st"));
    EXPECT_FALSE(endswith("test", "te"));
}

TEST(EndsWithTest, EmptySuffix) {
    // Empty suffix should always return true
    EXPECT_TRUE(endswith("hello", ""));
}

TEST(EndsWithTest, EmptyString) {
    // Empty string ends with empty suffix
    EXPECT_TRUE(endswith("", ""));
    // Empty string doesn't end with non-empty suffix
    EXPECT_FALSE(endswith("", "test"));
}

TEST(EndsWithTest, LongerSuffix) {
    // Suffix longer than string should return false
    EXPECT_FALSE(endswith("short", "too long suffix"));
}

// StartsWith Tests
TEST(StartsWithTest, Basic) {
    EXPECT_TRUE(startswith("hello world", "hello"));
    EXPECT_FALSE(startswith("hello world", "world"));
    EXPECT_TRUE(startswith("test", "te"));
    EXPECT_FALSE(startswith("test", "st"));
}

TEST(StartsWithTest, EmptyPrefix) {
    // Empty prefix should always return true
    EXPECT_TRUE(startswith("hello", ""));
}

TEST(StartsWithTest, EmptyString) {
    // Empty string starts with empty prefix
    EXPECT_TRUE(startswith("", ""));
    // Empty string doesn't start with non-empty prefix
    EXPECT_FALSE(startswith("", "test"));
}

TEST(StartsWithTest, LongerPrefix) {
    // Prefix longer than string should return false
    EXPECT_FALSE(startswith("short", "too long prefix"));
}

// LeadingWhiteSpace Tests
TEST(LeadingWhiteSpaceTest, Basic) {
    EXPECT_EQ(get_leading_white_space("  hello"), "  ");
    EXPECT_EQ(get_leading_white_space("\t\tworld"), "\t\t");
    EXPECT_EQ(get_leading_white_space(" \t \ttest"), " \t \t");
    EXPECT_EQ(get_leading_white_space("nowhitespace"), "");
}

TEST(LeadingWhiteSpaceTest, AllWhitespace) {
    EXPECT_EQ(get_leading_white_space("  \t  "), "  \t  ");
}

TEST(LeadingWhiteSpaceTest, EmptyString) {
    EXPECT_EQ(get_leading_white_space(""), "");
}

TEST(LeadingWhiteSpaceTest, MixedWhitespace) {
    EXPECT_EQ(get_leading_white_space(" \t\n\r\f\vtext"), " \t\n\r\f\v");
}

// Trim Tests
TEST(TrimTest, Basic) {
    EXPECT_EQ(trim("  hello  "), "hello");
    EXPECT_EQ(trim("\t\tworld\t\t"), "world");
    EXPECT_EQ(trim(" \t \ttest \t \t"), "test");
}

TEST(TrimTest, NoWhitespace) {
    EXPECT_EQ(trim("nowhitespace"), "nowhitespace");
}

TEST(TrimTest, AllWhitespace) {
    EXPECT_EQ(trim("  \t  "), "");
    EXPECT_EQ(trim(""), "");
}

TEST(TrimTest, OnlyLeadingWhitespace) {
    EXPECT_EQ(trim("  text"), "text");
}

TEST(TrimTest, OnlyTrailingWhitespace) {
    EXPECT_EQ(trim("text  "), "text");
}

TEST(TrimTest, MixedWhitespace) {
    EXPECT_EQ(trim(" \t\n\r\f\vtext \t\n\r\f\v"), "text");
}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
