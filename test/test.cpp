#include <clipboardxx.hpp>
#include <gtest/gtest.h>

#include "utils.hpp"

constexpr size_t kSmallTextSize = 100;
constexpr size_t kLargeTextSize = 10000;

class ClipboardTest : public testing::Test {
protected:
    void expect_clipboard_data(const std::string &text) {
        /* using another instance because in x11 linux same instance owns clipboard and will
           not send request and just uses internal memory to access clipboard data */

        const clipboardxx::clipboard clipboard;
        EXPECT_EQ(clipboard.paste(), text);
    }

    template <typename Container> void expect_no_duplicate_item(const Container &container) {
        for (auto iter = container.begin(); iter != container.end(); iter = std::next(iter))
            EXPECT_EQ(std::find(container.begin(), container.end(), *iter), iter);
    }

    const clipboardxx::clipboard m_clipboard;
    RandomGenerator m_random_generator;
};

TEST_F(ClipboardTest, RandomGeneratorGenerateDisplayableTextFiveSamplesMustBeDifferent) {
    std::vector<std::string> samples(5);
    std::generate(samples.begin(), samples.end(),
                  std::bind(&RandomGenerator::generate_random_displayable_text, m_random_generator, kSmallTextSize));
    expect_no_duplicate_item(samples);
}

TEST_F(ClipboardTest, RandomGeneratorGenerateBytesFiveSamplesMustBeDifferent) {
    std::vector<std::vector<uint8_t>> samples(5);
    std::generate(samples.begin(), samples.end(),
                  std::bind(&RandomGenerator::generate_random_bytes, m_random_generator, kSmallTextSize));
    expect_no_duplicate_item(samples);
}

TEST_F(ClipboardTest, CopyPasteSmallText) {
    const std::string random_text = m_random_generator.generate_random_displayable_text(kSmallTextSize);
    m_clipboard.copy(random_text);
    expect_clipboard_data(random_text);
}

TEST_F(ClipboardTest, CopyPasteLargeText) {
    const std::string random_text = m_random_generator.generate_random_displayable_text(kLargeTextSize);
    m_clipboard.copy(random_text);
    expect_clipboard_data(random_text);
}

TEST_F(ClipboardTest, CopyPasteWithOperatorOverload) {
    const std::string text = "hello";
    m_clipboard << text;

    std::string result;
    m_clipboard >> result;
    EXPECT_EQ(text, result);
}

TEST_F(ClipboardTest, PasteWhenYouAreClipboardOwner) {
    const std::string random_text = m_random_generator.generate_random_displayable_text(kSmallTextSize);
    m_clipboard.copy(random_text);
    EXPECT_EQ(m_clipboard.paste(), random_text);
}

TEST_F(ClipboardTest, PasteTextIsEmptyWhenNoDataIsAvailable) {
    // become clipboard owner and then close clipboard
    {
        clipboardxx::clipboard temp_clipboard;
        temp_clipboard.copy("");
    }

    EXPECT_EQ(m_clipboard.paste(), "");
}

#ifdef LINUX

TEST_F(ClipboardTest, ClipboardDataGetLostAfterClipboardGoesOutOfScopeInX11Linux) {
    const std::string text = "hello";

    {
        clipboardxx::clipboard clipboard;
        clipboard.copy(text);
    }

    EXPECT_EQ(m_clipboard.paste(), "");
}

#elif defined(WINDOWS)

TEST_F(ClipboardTest, ClipboardDataRemainsAfterClipboardGoesOutOfScopeInWindows) {
    const std::string text = "hello";

    {
        clipboardxx::clipboard clipboard;
        clipboard.copy(text);
    }

    EXPECT_EQ(m_clipboard.paste(), text);
}

#endif
