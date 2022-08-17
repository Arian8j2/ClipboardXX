#include <clipboardxx.hpp>
#include <gtest/gtest.h>

#include "utils.hpp"

constexpr size_t SMALL_TEXT_SIZE = 100;
constexpr size_t LARGE_TEXT_SIZE = 10000;

class ClipboardTest : public testing::Test {
protected:
    void expect_clipboard_data(const std::string &text) {
        /* using another instance because same instance owns clipboard and will not send request
           and just uses internal memory to access clipboard data */

        const clipboardxx::clipboard clipboard;
        EXPECT_EQ(clipboard.paste(), text);
    }

    template <typename Container>
    void expect_no_duplicate_item(const Container &container) {
        for (auto iter = container.begin(); iter != container.end(); iter = std::next(iter))
            EXPECT_EQ(std::find(container.begin(), container.end(), *iter), iter);
    }

    clipboardxx::clipboard m_clipboard;
    RandomGenerator m_random_generator;
};

TEST_F(ClipboardTest, RandomGeneratorGenerateDisplayableTextFiveSamplesMustBeDifferent) {
    std::vector<std::string> samples(5);
    std::generate(samples.begin(), samples.end(),
            std::bind(&RandomGenerator::generate_random_displayable_text, m_random_generator, SMALL_TEXT_SIZE));
    expect_no_duplicate_item(samples); 
}

TEST_F(ClipboardTest, RandomGeneratorGenerateBytesFiveSamplesMustBeDifferent) {
    std::vector<std::vector<uint8_t>> samples(5);
    std::generate(samples.begin(), samples.end(),
            std::bind(&RandomGenerator::generate_random_bytes, m_random_generator, SMALL_TEXT_SIZE));
    expect_no_duplicate_item(samples);
}

TEST_F(ClipboardTest, CopyPasteSmallText) {
    const std::string random_text = m_random_generator.generate_random_displayable_text(SMALL_TEXT_SIZE);
    m_clipboard.copy(random_text);
    expect_clipboard_data(random_text);
}

TEST_F(ClipboardTest, CopyPasteTextContainingNullAtMiddle) {
    const std::vector<uint8_t> bytes({'a', '\0', 'b'});
    const std::string string_with_null(bytes.begin(), bytes.end());
    EXPECT_EQ(string_with_null.at(1), '\0');

    m_clipboard.copy(string_with_null);
    expect_clipboard_data(string_with_null);
}

TEST_F(ClipboardTest, CopyPasteLargeText) {
    const std::string random_text = m_random_generator.generate_random_displayable_text(LARGE_TEXT_SIZE);
    m_clipboard.copy(random_text);
    expect_clipboard_data(random_text);
}

TEST_F(ClipboardTest, PasteWhenYouAreClipboardOwner) {
    const std::string random_text = m_random_generator.generate_random_displayable_text(SMALL_TEXT_SIZE);
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

TEST_F(ClipboardTest, ClipboardDataGetLostAfterClipboardGoesOutOfScopeInX11Linux) {
    const std::string text = "hello";

    {
        clipboardxx::clipboard clipboard;
        clipboard.copy(text);
    }

    EXPECT_EQ(m_clipboard.paste(), "");
}
