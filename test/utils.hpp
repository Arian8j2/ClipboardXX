#include <ctime>
#include <functional>
#include <random>
#include <vector>

constexpr const char DISPLAYABLE_CHARACTERS[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890#$&@";

class RandomGenerator {
public:
    RandomGenerator()
        : m_engine(std::default_random_engine(std::time(nullptr))),
          m_displayable_char_index_distro(
              std::uniform_int_distribution<uint8_t>(sizeof(char) * sizeof(DISPLAYABLE_CHARACTERS))) {}

    std::vector<uint8_t> generate_random_bytes(size_t size) {
        std::vector<uint8_t> result(size);
        std::generate(result.begin(), result.end(), std::bind(m_char_distro, std::ref(m_engine)));
        return result;
    }

    std::string generate_random_displayable_text(size_t size) {
        std::string result(size, ' ');
        for (char &character : result)
            character = DISPLAYABLE_CHARACTERS[m_displayable_char_index_distro(m_engine)];
        return result;
    }

    uint16_t generate_random_16_bit_number() { return m_uint16_distro(m_engine); }

private:
    std::default_random_engine m_engine;
    std::uniform_int_distribution<uint16_t> m_uint16_distro;
    std::uniform_int_distribution<uint8_t> m_char_distro;
    std::uniform_int_distribution<uint8_t> m_displayable_char_index_distro;
};
