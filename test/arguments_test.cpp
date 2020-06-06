// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "arguments.hpp"

#include <gtest/gtest.h>

TEST(ArgumentsTest, Iterate)
{
    char* testArgs[] = {const_cast<char*>("one"), const_cast<char*>("two"),
                        const_cast<char*>("three")};
    const int argsNum = sizeof(testArgs) / sizeof(testArgs[0]);

    Arguments args(argsNum, testArgs);

    for (int i = 0; i < argsNum; ++i)
    {
        ASSERT_THROW(args.expectEnd(), std::invalid_argument);
        EXPECT_TRUE(args.peek());
        EXPECT_STREQ(args.asText(), testArgs[i]);
    }

    EXPECT_FALSE(args.peek());
    ASSERT_THROW(args.asText(), std::invalid_argument);
    args.expectEnd();
}

TEST(ArgumentsTest, Numeric)
{
    char* testArgs[] = {const_cast<char*>("0"),
                        const_cast<char*>("100"),
                        const_cast<char*>("-100"),
                        const_cast<char*>("12345678987654321123456789"),
                        const_cast<char*>("12abc"),
                        const_cast<char*>("abc"),
                        const_cast<char*>("")};
    const int argsNum = sizeof(testArgs) / sizeof(testArgs[0]);

    Arguments args(argsNum, testArgs);

    EXPECT_EQ(args.asNumber(), 0);
    EXPECT_EQ(args.asNumber(), 100);
    ASSERT_THROW(args.asNumber(), std::invalid_argument);
    ASSERT_THROW(args.asNumber(), std::invalid_argument);
    ASSERT_THROW(args.asNumber(), std::invalid_argument);
    ASSERT_THROW(args.asNumber(), std::invalid_argument);
    ASSERT_THROW(args.asNumber(), std::invalid_argument);
}

TEST(ArgumentsTest, Action)
{
    char* testArgs[] = {const_cast<char*>("add"), const_cast<char*>("del"),
                        const_cast<char*>("addd"), const_cast<char*>("ad"),
                        const_cast<char*>("")};
    const int argsNum = sizeof(testArgs) / sizeof(testArgs[0]);

    Arguments args(argsNum, testArgs);

    EXPECT_EQ(args.asAction(), Action::add);
    EXPECT_EQ(args.asAction(), Action::del);
    ASSERT_THROW(args.asAction(), std::invalid_argument);
    ASSERT_THROW(args.asAction(), std::invalid_argument);
    ASSERT_THROW(args.asAction(), std::invalid_argument);
}

TEST(ArgumentsTest, Toggle)
{
    char* testArgs[] = {const_cast<char*>("enable"), const_cast<char*>("disable"),
                        const_cast<char*>("enablee"), const_cast<char*>("en"),
                        const_cast<char*>("")};
    const int argsNum = sizeof(testArgs) / sizeof(testArgs[0]);

    Arguments args(argsNum, testArgs);

    EXPECT_EQ(args.asToggle(), Toggle::enable);
    EXPECT_EQ(args.asToggle(), Toggle::disable);
    ASSERT_THROW(args.asToggle(), std::invalid_argument);
    ASSERT_THROW(args.asToggle(), std::invalid_argument);
    ASSERT_THROW(args.asToggle(), std::invalid_argument);
}

TEST(ArgumentsTest, MacAddress)
{
    char* testArgs[] = {
        const_cast<char*>("01:23:45:67:89:ab"),
        const_cast<char*>("01.23.45-67-89:ab"),
        const_cast<char*>("qq:22:33:44:55:66"),
        const_cast<char*>("text"),
        const_cast<char*>("")};
    const int argsNum = sizeof(testArgs) / sizeof(testArgs[0]);

    Arguments args(argsNum, testArgs);

    EXPECT_EQ(args.asMacAddress(), testArgs[0]);
    ASSERT_THROW(args.asMacAddress(), std::invalid_argument);
    ASSERT_THROW(args.asMacAddress(), std::invalid_argument);
    ASSERT_THROW(args.asMacAddress(), std::invalid_argument);
    ASSERT_THROW(args.asMacAddress(), std::invalid_argument);
}

TEST(ArgumentsTest, IpAddress)
{
    char* testArgs[] = {
        const_cast<char*>("127.0.0.1"),
        const_cast<char*>("127.0.256.1"),
        const_cast<char*>("127.0.0,1"),
        const_cast<char*>("127.0.0"),
        const_cast<char*>("127.0.0.1.2"),
        const_cast<char*>("2001:0db8:85a3:0000:0000:8a2e:0370:7334"),
        const_cast<char*>("::"),
        const_cast<char*>("text"),
        const_cast<char*>("")};
    const int argsNum = sizeof(testArgs) / sizeof(testArgs[0]);

    Arguments args(argsNum, testArgs);

    EXPECT_EQ(args.asIpAddress(), std::make_tuple(IpVer::v4, testArgs[0]));
    ASSERT_THROW(args.asIpAddress(), std::invalid_argument);
    ASSERT_THROW(args.asIpAddress(), std::invalid_argument);
    ASSERT_THROW(args.asIpAddress(), std::invalid_argument);
    ASSERT_THROW(args.asIpAddress(), std::invalid_argument);
    EXPECT_EQ(args.asIpAddress(), std::make_tuple(IpVer::v6, testArgs[5]));
    EXPECT_EQ(args.asIpAddress(), std::make_tuple(IpVer::v6, testArgs[6]));
    ASSERT_THROW(args.asIpAddress(), std::invalid_argument);
    ASSERT_THROW(args.asIpAddress(), std::invalid_argument);
}

TEST(ArgumentsTest, IpAddrMask)
{
    char* testArgs[] = {const_cast<char*>("127.0.0.1/8"),
                        const_cast<char*>("127.0.0.1/0"),
                        const_cast<char*>("127.0.256.1/8"),
                        const_cast<char*>("127.0.0.1"),
                        const_cast<char*>("127.0.0.1/"),
                        const_cast<char*>("127.0.0/8"),
                        const_cast<char*>("2001:db8:a::123/64"),
                        const_cast<char*>("text"),
                        const_cast<char*>("")};
    const int argsNum = sizeof(testArgs) / sizeof(testArgs[0]);

    Arguments args(argsNum, testArgs);

    EXPECT_EQ(args.asIpAddrMask(), std::make_tuple(IpVer::v4, "127.0.0.1", 8));
    ASSERT_THROW(args.asIpAddrMask(), std::invalid_argument);
    ASSERT_THROW(args.asIpAddrMask(), std::invalid_argument);
    ASSERT_THROW(args.asIpAddrMask(), std::invalid_argument);
    ASSERT_THROW(args.asIpAddrMask(), std::invalid_argument);
    ASSERT_THROW(args.asIpAddrMask(), std::invalid_argument);
    EXPECT_EQ(args.asIpAddrMask(),
              std::make_tuple(IpVer::v6, "2001:db8:a::123", 64));
    ASSERT_THROW(args.asIpAddrMask(), std::invalid_argument);
    ASSERT_THROW(args.asIpAddrMask(), std::invalid_argument);
}
