#include <array>
#include <ctime>
#include <functional>
#include <random>
#include <vector>

constexpr std::array<char, 67> kDisplayableCharacters = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
    'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '#', '$', '&', '@'};

class RandomGenerator {
public:
    RandomGenerator()
        : m_engine(std::default_random_engine(std::time(nullptr))),
          m_displayable_char_index_distro(
              std::uniform_int_distribution<uint8_t>(0, kDisplayableCharacters.size() - 1)) {}

    std::vector<uint8_t> generate_random_bytes(size_t size) {
        std::vector<uint8_t> result(size);
        std::generate(result.begin(), result.end(), std::bind(m_char_distro, std::ref(m_engine)));
        return result;
    }

    std::string generate_random_displayable_text(size_t size) {
        std::string result(size, ' ');
        for (char &character : result)
            character = kDisplayableCharacters.at(m_displayable_char_index_distro(m_engine));
        return result;
    }

private:
    std::default_random_engine m_engine;
    std::uniform_int_distribution<uint8_t> m_char_distro;
    std::uniform_int_distribution<uint8_t> m_displayable_char_index_distro;
};
