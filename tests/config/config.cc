#include "config/config.h"
#include "containers/str.h"

#include "gtest/gtest.h"

using namespace xynq;

TEST(ConfigTest, LoadFromArgs) {
    const char *args[] = {
        "/key1",
        "\"Value1\"",
        "/key2",
        "325",
        "/key3",
        "32.098"
    };

    auto result = Config::LoadFromArgs(sizeof(args)/sizeof(args[0]), args);
    ASSERT_TRUE(result.IsRight());

    const Config &conf = result.Right();
    ASSERT_TRUE(conf.Get<CStrSpan>("key1").IsRight());
    CStrSpan val = conf.Get<CStrSpan>("key1").Right();
    ASSERT_STREQ(val.CStr(), "Value1");

    ASSERT_TRUE(conf.Get<int>("key2").IsRight());
    ASSERT_EQ(conf.Get<int>("key2").Right(), 325);
    ASSERT_TRUE(conf.Get<double>("key3").IsRight());
    ASSERT_EQ(conf.Get<double>("key3").Right(), 32.098);
}

TEST(ConfigTest, LoadFromFile) {
    Str temp_dir = "/tmp";
    Str temp_filename = temp_dir;
    temp_filename += "/__config.test.conf";

    FILE *fp = ::fopen(temp_filename.c_str(), "wb+");
    XYAssert(fp != nullptr);

    char code[] = R"(
        ; Three keys total.
        ; String value.
        (key1   "str val")  ; Key 1

        ; Integer value.
        (key2   325)        ; Key 2

        ; Double value.
        (key3   321.75)     ; Key 3
    )";

    ::fputs(&code[0], fp);
    ::fclose(fp);

    auto result = Config::LoadFromFile(temp_filename.c_str());
    ASSERT_TRUE(result.IsRight());

    const Config &conf = result.Right();
    ASSERT_TRUE(conf.Get<CStrSpan>("key1").IsRight());
    CStrSpan val = conf.Get<CStrSpan>("key1").Right();
    ASSERT_STREQ(val.CStr(), "str val");

    ASSERT_TRUE(conf.Get<int>("key2").IsRight());
    ASSERT_EQ(conf.Get<int>("key2").Right(), 325);
    ASSERT_TRUE(conf.Get<double>("key3").IsRight());
    ASSERT_EQ(conf.Get<double>("key3").Right(), 321.75);

    ::unlink(temp_filename.c_str());
}

TEST(ConfigTest, LoadFromBuffer) {
    char code[] = R"(
        ; Three keys total.
        ; String value.
        (key1   "str val")  ; Key 1

        ; Integer value.
        (key2   325)        ; Key 2

        ; Double value.
        (key3   321.75)     ; Key 3
    )";

    auto result = Config::LoadFromBuffer(code);

    ASSERT_TRUE(result.IsRight());

    const Config &conf = result.Right();
    CStrSpan val = conf.Get<CStrSpan>("key1").Right();
    ASSERT_STREQ(val.CStr(), "str val");
    ASSERT_EQ(conf.Get<int>("key2").Right(), 325);
    ASSERT_EQ(conf.Get<double>("key3").Right(), 321.75);
}

TEST(ConfigTest, Int) {
    char code[] = R"(
        (key1 325)
    )";

    auto result = Config::LoadFromBuffer(code);

    ASSERT_TRUE(result.IsRight());

    const Config &conf = result.Right();
    ASSERT_EQ(conf.Get<int>("key1").Right(), 325);
}

TEST(ConfigTest, Double) {
    char code[] = R"(
        (key1 325.75)
    )";

    auto result = Config::LoadFromBuffer(code);

    ASSERT_TRUE(result.IsRight());

    const Config &conf = result.Right();
    ASSERT_EQ(conf.Get<double>("key1").Right(), 325.75);
}

TEST(ConfigTest, Bool) {
    char code[] = R"(
        (key1 Yes)
        (key2 No)
    )";

    auto result = Config::LoadFromBuffer(code);

    ASSERT_TRUE(result.IsRight());

    const Config &conf = result.Right();
    ASSERT_EQ(conf.Get<bool>("key1").Right(), true);
    ASSERT_EQ(conf.Get<bool>("key2").Right(), false);
}

TEST(ConfigTest, String) {
    char code[] = R"(
        (key1 "str val")
    )";

    auto result = Config::LoadFromBuffer(code);

    ASSERT_TRUE(result.IsRight());

    const Config &conf = result.Right();
    CStrSpan val = conf.Get<CStrSpan>("key1").Right();
    ASSERT_STREQ(val.CStr(), "str val");
}

TEST(ConfigTest, InvalidType) {
    char code[] = R"(
        (key1 325)
    )";

    auto result = Config::LoadFromBuffer(code);

    ASSERT_TRUE(result.IsRight());

    const Config &conf = result.Right();
    ASSERT_TRUE(conf.Get<bool>("key1").IsLeft());
    ASSERT_EQ(conf.Get<bool>("key1").Left(), ConfigKeyError::InvalidType);
}

TEST(ConfigTest, NestedKeys) {
    char code[] = R"(
        ;
        ; Nested keys.
        ;
        (key1                           ; Key 1
            (key2 325)                  ; Key 2
            (key3                       ; Key 3
                (key4 25.25)            ; Key 4
                (key5 "str value")))    ; Key 5
    )";

    auto result = Config::LoadFromBuffer(code);

    ASSERT_TRUE(result.IsRight());

    const Config &conf = result.Right();
    ASSERT_EQ(conf.Get<int>("key1.key2").Right(), 325);
    ASSERT_EQ(conf.Get<double>("key1.key3.key4").Right(), 25.25);
    CStrSpan str_value = conf.Get<CStrSpan>("key1.key3.key5").Right();
    ASSERT_STREQ(str_value.CStr(), "str value");
}

TEST(ConfigTest, Defaults) {
    Config conf;

    CStrSpan value = conf.Get<CStrSpan>("key1").FoldLeft([]() {
        return "Default Str";
    });

    ASSERT_STREQ(value.CStr(), "Default Str");
}

TEST(ConfigTest, Merge) {
    char code1[] = "(x 1) (y 2)";
    char code2[] = "(y 3) (z 4)";

    auto result1 = Config::LoadFromBuffer(code1);
    auto result2 = Config::LoadFromBuffer(code2);

    ASSERT_TRUE(result1.IsRight());
    ASSERT_TRUE(result2.IsRight());

    Config &conf1 = result1.Right();
    Config &conf2 = result2.Right();

    // Merge conf1 and conf2 into conf.
    Config conf = Config::Merge(std::move(conf1), std::move(conf2));

    ASSERT_TRUE(conf.Get<int>("x").IsRight());
    ASSERT_TRUE(conf.Get<int>("y").IsRight());
    ASSERT_TRUE(conf.Get<int>("z").IsRight());

    ASSERT_EQ(conf.Get<int>("x").Right(), 1);
    ASSERT_EQ(conf.Get<int>("y").Right(), 3);
    ASSERT_EQ(conf.Get<int>("z").Right(), 4);
}

TEST(ConfigTest, Enumerate) {
    char code[] = R"(
        (key1   "str val")  ; Key 1
        (key2   325)        ; Key 2
        (key3   321.75)     ; Key 3
    )";

    auto result = Config::LoadFromBuffer(code);

    ASSERT_TRUE(result.IsRight());
    const Config &conf = result.Right();
    int total = 0;

    conf.Enumerate([&](CStrSpan key, CStrSpan value) {
        ++total;

        if (key == "key1") {
            ASSERT_TRUE(value == "str val");
        } else if (key == "key2") {
            ASSERT_TRUE(value == "325");
        } else if (key == "key3") {
            ASSERT_EQ(::strncmp(value.CStr(), "321.75", 6), 0);
        } else {
            ASSERT_TRUE(false);
        }
    });

    ASSERT_EQ(total, 3);
}

TEST(ConfigTest, List) {
    char code[] = R"(
        (key
            "value 1"
            325
            325.65)
    )";

    auto result = Config::LoadFromBuffer(code);

    ASSERT_TRUE(result.IsRight());
    const Config &conf = result.Right();

    auto list_result = conf.GetList("key");
    ASSERT_TRUE(list_result.IsRight());

    const ConfigList &values = list_result.Right();
    int i = 1;
    for (const auto &it : values) {
        switch (i) {
            case 1: {
                CStrSpan str_val = it.Get<CStrSpan>().Right().CStr();
                ASSERT_TRUE(str_val == "value 1");
                break;
            }
            case 2: {
                int int_val{it.Get<int>().Right()};
                ASSERT_EQ(int_val, 325);
                break;
            }
            case 3: {
                double dbl_val{it.Get<double>().Right()};
                ASSERT_EQ(dbl_val, 325.65);
                break;
            }
        }
        ++i;
    }

    ASSERT_EQ(i, 4);
}

TEST(ConfigTest, ListAsArray) {
    char code[] = R"(
        (key 0 1 2 3 4 5 6 7 8 9)
    )";

    auto result = Config::LoadFromBuffer(code);

    ASSERT_TRUE(result.IsRight());
    const Config &conf = result.Right();

    auto list_result = conf.GetList("key");
    ASSERT_TRUE(list_result.IsRight());

    const auto &arr = list_result.Right().AsArray<int>();
    ASSERT_TRUE(arr.HasValue());
    ASSERT_EQ(arr.Value().size(), 10u);
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(i, arr.Value()[i]);
    }
}

TEST(ConfigTest, DefaultList) {
    Config conf;

    auto values = conf.GetList("key")
        .RightOrDefault(ConfigList::Make("value 1", 25, 325.65));

    int i = 1;
    for (const auto &val : values) {
        switch (i) {
            case 1: {
                CStrSpan str_val = val.Get<CStrSpan>().Right().CStr();
                ASSERT_TRUE(str_val == "value 1");
                break;
            }
            case 2: {
                int int_val{val.Get<int>().Right()};
                ASSERT_EQ(int_val, 25);
                break;
            }
            case 3: {
                double dbl_val{val.Get<double>().Right()};
                ASSERT_EQ(dbl_val, 325.65);
                break;
            }
        }
        ++i;
    }

    ASSERT_EQ(i, 4);
}


TEST(ConfigTest, NestedListsError1) {
    char code[] = R"(
        (key1
                "hello"
                (key2 123))
    )";

    auto result = Config::LoadFromBuffer(code);
    ASSERT_TRUE(result.IsLeft());
}

TEST(ConfigTest, NestedListsError2) {
    char code[] = R"(
        (key1
                (key2 123)
                "value")
    )";

    auto result = Config::LoadFromBuffer(code);
    ASSERT_TRUE(result.IsLeft());
}
