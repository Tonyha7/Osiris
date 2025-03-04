#pragma once

#include <span>

#include "ConfigStringConversionState.h"

class ConfigFromString {
public:
    ConfigFromString(std::span<const char8_t> buffer, ConfigStringConversionState& readState) noexcept
        : buffer{buffer}
        , readState{readState}
    {
    }

    void beginRoot() noexcept
    {
        if (shouldReadThisObject() && skipWhitespaces() && readChar(u8'{'))
            increaseConversionNestingLevel();
        increaseNestingLevel();
    }

    [[nodiscard]] std::size_t endRoot() noexcept
    {
        if (shouldReadThisObject() && skipWhitespaces() && readChar(u8'}'))
            ++readState.indexInNestingLevel[--readState.nestingLevel];
        decreaseNestingLevel();
        readState.offset += readIndex;
        return readIndex;
    }

    void beginObject(const char8_t* id) noexcept
    {
        if (shouldReadThisObject()) {
            const auto previousReadIndex = readIndex;
            if (readCommaAfterPreviousElement() && readKey(id) && skipWhitespaces() && readChar(u8'{'))
                increaseConversionNestingLevel();
            else
                readIndex = previousReadIndex;
        }
        increaseNestingLevel();
    }

    void endObject() noexcept
    {
        if (shouldReadThisObject() && skipWhitespaces() && readChar(u8'}'))
            --readState.nestingLevel;
        decreaseNestingLevel();
    }

    void boolean(const char8_t* key, auto&& valueSetter, auto&& /* valueGetter */) noexcept
    {
        if (shouldReadThisObject()) {
            const auto previousReadIndex = readIndex;
            if (bool parsedBool{}; readCommaAfterPreviousElement() && readKey(key) && skipWhitespaces() && parseBool(parsedBool)) {
                valueSetter(parsedBool);
                readState.indexInNestingLevel[readState.nestingLevel] = 0;
            } else {
                readIndex = previousReadIndex;
            }
        }
        ++indexInNestingLevel[nestingLevel];
    }


    void uint(const char8_t* key, auto&& valueSetter, auto&& /* valueGetter */) noexcept
    {
        if (shouldReadThisObject()) {
            const auto previousReadIndex = readIndex;
            std::uint64_t parsedUint{0};
            if (readCommaAfterPreviousElement() && readKey(key) && skipWhitespaces() && parseUint(parsedUint)) {
                valueSetter(parsedUint);
                readState.indexInNestingLevel[readState.nestingLevel] = 0;
            } else {
                readIndex = previousReadIndex;
            }
        }
        ++indexInNestingLevel[nestingLevel];
    }

private:
    [[nodiscard]] bool readCommaAfterPreviousElement() noexcept
    {
        if (readState.indexInNestingLevel[readState.nestingLevel] != config_params::kInvalidObjectIndex) {
            skipWhitespaces();
            return readChar(u8',');
        }
        return true;
    }

    [[nodiscard]] bool readKey(const char8_t* key) noexcept
    {
        return skipWhitespaces() && readChar(u8'"') && readString(key) && readChar(u8'"') && skipWhitespaces() && readChar(u8':');
    }

    void increaseNestingLevel() noexcept
    {
        assert(nestingLevel < config_params::kMaxNestingLevel);
        indexInNestingLevel[++nestingLevel] = 0;
    }

    void increaseConversionNestingLevel() noexcept
    {
        assert(readState.nestingLevel < config_params::kMaxNestingLevel);
        readState.indexInNestingLevel[readState.nestingLevel] = indexInNestingLevel[nestingLevel];
        readState.indexInNestingLevel[++readState.nestingLevel] = config_params::kInvalidObjectIndex;
    }

    void decreaseNestingLevel() noexcept
    {
        assert(nestingLevel > 0);
        assert(indexInNestingLevel[nestingLevel - 1] < config_params::kMaxObjectIndex);
        ++indexInNestingLevel[--nestingLevel];
    }

    [[nodiscard]] bool shouldReadThisObject() const noexcept
    {
        if (readState.nestingLevel != nestingLevel)
            return false;
        for (auto i = 0; i < readState.nestingLevel; ++i) {
            if (indexInNestingLevel[i] != readState.indexInNestingLevel[i])
                return false;
        }
        return true;
    }

    [[nodiscard]] bool parseUint(std::uint64_t& result) noexcept
    {
        result = 0;
        bool parsedAtLeastOneDigit = false;
        while (readIndex < buffer.size()) {
            if (const char c = buffer[readIndex]; c >= u8'0' && c <= u8'9') {
                parsedAtLeastOneDigit = true;
                ++readIndex;
                const auto lastResult = result;
                result *= 10;
                result += c - u8'0';
                if (result < lastResult)
                    return false;
            } else {
                return parsedAtLeastOneDigit;
            }
        }
        return false;
    }

    [[nodiscard]] bool parseBool(bool& result) noexcept
    {
        const auto previousReadIndex = readIndex;
        if (readString(u8"true")) {
            result = true;
            return true;
        }

        readIndex = previousReadIndex;
        if (readString(u8"false")) {
            result = false;
            return true;
        }

        readIndex = previousReadIndex;
        return false;
    }

    [[nodiscard]] static constexpr bool isWhitespace(char8_t c) noexcept
    {
        switch (c) {
        case u8' ':
        case u8'\t':
        case u8'\n':
        case u8'\r':
            return true;
        default:
            return false;
        }
    }

    bool skipWhitespaces() noexcept
    {
        while (readIndex < buffer.size() && isWhitespace(buffer[readIndex]))
            ++readIndex;
        return true; 
    }

    [[nodiscard]] bool readChar(char8_t c) noexcept
    {
        if (readIndex < buffer.size() && buffer[readIndex] == c) {
            ++readIndex;
            return true;
        }
        return false;
    }

    [[nodiscard]] bool readString(const char8_t* str) noexcept
    {
        while (*str && readIndex < buffer.size()) {
            if (*str++ != buffer[readIndex++])
                return false;
        }
        return *str == 0;
    }

    std::span<const char8_t> buffer;
    std::size_t readIndex{0};
    ConfigStringConversionState& readState;
    std::array<config_params::ObjectIndexType, config_params::kMaxNestingLevel + 1> indexInNestingLevel{};
    config_params::NestingLevelIndexType nestingLevel{0};
};
