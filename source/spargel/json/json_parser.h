#pragma once

#include "spargel/base/either.h"
#include "spargel/base/optional.h"
#include "spargel/base/string.h"
#include "spargel/base/string_view.h"
#include "spargel/json/json_value.h"

namespace spargel::json {

    // Copied from `codec::CodecError`.
    class JsonParseError {
    public:
        JsonParseError(const base::String& message) : message_(message) {}
        JsonParseError(base::StringView message) : message_(message) {}

        const base::String& message() { return message_; }

        friend JsonParseError operator+(const JsonParseError& error,
                                        base::StringView str) {
            return JsonParseError((error.message_ + str).view());
        }

        friend JsonParseError operator+(const JsonParseError& error, char ch) {
            return JsonParseError((error.message_ + ch).view());
        }

        friend JsonParseError operator+(const JsonParseError& error1,
                                        const JsonParseError& error2) {
            return JsonParseError((error1.message_ + error2.message_).view());
        }

    private:
        base::String message_;
    };

    class JsonParser {
    public:
        struct Cursor {
            const char* cur;
            const char* end;

            bool isEnd() const { return cur >= end; }

            char peek() const {
                if (isEnd()) return 0;
                return *cur;
            }

            void advance(int steps = 1) { cur += steps; }

            char consumeChar() {
                auto ch = peek();
                advance();
                return ch;
            }

            /*
             * These funtions will move the cursor if succeeds,
             * otherwise the cursor will not move.
             */
            bool tryEatChar(char ch);
            bool tryEatBytes(const u8* bytes, usize count);
            bool tryEatString(const char* str);
        };

        Cursor cursor;

        static base::Either<JsonValue, JsonParseError> parse(char const* s,
                                                             usize l);
        static base::Either<JsonValue, JsonParseError> parse(
            base::StringView s) {
            return parse(s.data(), s.length());
        }

        void eatWhitespaces();
        base::Either<JsonValue, JsonParseError> parseValue();
        base::Either<JsonObject, JsonParseError> parseObject();
        base::Either<JsonArray, JsonParseError> parseArray();
        base::Either<JsonString, JsonParseError> parseString();
        base::Optional<JsonParseError> parseInteger(JsonNumber& number,
                                                    bool& minus);
        base::Optional<JsonParseError> parseFraction(JsonNumber& number);
        base::Optional<JsonParseError> parseExponent(JsonNumber& number);
        base::Either<JsonNumber, JsonParseError> parseNumber();
        base::Either<JsonBoolean, JsonParseError> parseBoolean();
        base::Optional<JsonParseError> parseNull();
        base::Either<JsonObject, JsonParseError> parseMembers();
        base::Optional<JsonParseError> parseMember(JsonString& key,
                                                   JsonValue& value);
        base::Either<JsonArray, JsonParseError> parseElements();
        base::Either<JsonValue, JsonParseError> parseElement();
    };

    // base::Either<JsonValue, JsonParseError> parseJson(const char* str,
    //                                                   usize length);
    // inline base::Either<JsonValue, JsonParseError> parseJson(
    //     base::StringView s) {
    //     return parseJson(s.data(), s.length());
    // }
}  // namespace spargel::json
