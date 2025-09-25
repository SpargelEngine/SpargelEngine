#include "spargel/json/json_parser.h"

#include "spargel/base/either.h"
#include "spargel/base/optional.h"
#include "spargel/base/string_view.h"
#include "spargel/base/trace.h"
#include "spargel/json/json_value.h"

// libm
#include <math.h>

namespace spargel::json {

    using namespace base::literals;

    using base::String;

    using base::Either;
    using base::Left;
    using base::Right;

    using base::makeOptional;
    using base::nullopt;
    using base::Optional;

    namespace {

        const auto UNEXPECTED_END = JsonParseError("unexpected end"_sv);

        String char2hex(char ch) {
            const char hexDigits[] = "0123456789abcdef";
            return String(hexDigits[(ch >> 4) & 0xf]) + hexDigits[ch & 0xf];
        }

        void appendUtf8(base::vector<char>& chars, u32 code) {
            if (code <= 0x7f) {
                chars.emplace((char)code);
            } else if (code <= 0x7ff) {
                chars.emplace((char)(0xc0 | (code >> 6)));
                chars.emplace((char)(0x80 | (code & 0x3f)));
            } else if (code <= 0xffff) {
                chars.emplace((char)(0xe0 | (code >> 12)));
                chars.emplace((char)(0x80 | ((code >> 6) & 0x3f)));
                chars.emplace((char)(0x80 | (code & 0x3f)));
            } else if (code <= 0x10ffff) {
                chars.emplace((char)(0xf0 | (code >> 18)));
                chars.emplace((char)(0x80 | ((code >> 12) & 0x3f)));
                chars.emplace((char)(0x80 | ((code >> 6) & 0x3f)));
                chars.emplace((char)(0x80 | (code & 0x3f)));
            }
        }

    }  // namespace

    /*
     * The syntax descriptions in the following comments are from:
     *   https://www.json.org/json-en.html
     * The actual syntax we are using is slightly different from it.
     */

    /*
     * ws:
     *   ""
     *   '0020' ws
     *   '000A' ws
     *   '000D' ws
     *   '0009' ws
     */
    void JsonParser::eatWhitespaces() {
        spargel_trace_scope("eatWhitespaces");

        while (!cursor.isEnd()) {
            char ch = (char)cursor.peek();
            if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t')
                cursor.advance();
            else
                break;
        }
    }

    /*
     * value:
     *   object
     *   array
     *   string
     *   number
     *   "true"
     *   "false"
     *   "null"
     */
    Either<JsonValue, JsonParseError> JsonParser::parseValue() {
        spargel_trace_scope("parseValue");

        if (cursor.isEnd()) return Right(JsonParseError("expected a value"_sv));

        char ch = (char)cursor.peek();
        switch (ch) {
        case '{':
            /* object */
            return parseObject();
        case '[':
            /* array */
            return parseArray();
        case '"':
            /* string */
            return parseString();
        case 't':
        case 'f':
            /* boolean: true/false */
            return parseBoolean();
        case 'n': {
            /* null */
            auto result = parseNull();
            if (result.hasValue()) {
                return Right(base::move(result.value()));
            } else {
                return Left(JsonValue());
            }
        }
        default:
            /* number */
            if ((ch >= '0' && ch <= '9') || ch == '-') {
                return parseNumber();
            } else {
                return Right(JsonParseError(String("unexpected character: '") +
                                            ch + '\''));
            }
        }
    }

    /*
     * object:
     *   '{' ws '}'
     *   '{' members '}'
     */
    Either<JsonObject, JsonParseError> JsonParser::parseObject() {
        spargel_trace_scope("parseObject");

        // '{' ws
        if (!cursor.tryEatChar('{'))
            return Right(JsonParseError("expected a value"_sv));
        eatWhitespaces();
        if (cursor.isEnd()) return Right(JsonParseError(UNEXPECTED_END));

        // '}'
        if (cursor.tryEatChar('}')) return Left(JsonObject());

        // members '}'
        auto result = parseMembers();
        if (result.isRight()) return result;
        if (!cursor.tryEatChar('}'))
            return Right(JsonParseError("expected '}'"_sv));

        return result;
    }

    /*
     * array:
     *   '[' ws ']'
     *   '[' elements ']'
     */
    Either<JsonArray, JsonParseError> JsonParser::parseArray() {
        spargel_trace_scope("parseArray");

        // '[' ws
        if (!cursor.tryEatChar('['))
            return Right(JsonParseError("expected '['"_sv));
        eatWhitespaces();
        if (cursor.isEnd()) return Right(JsonParseError(UNEXPECTED_END));

        // ']'
        if (cursor.tryEatChar(']')) return Left(JsonArray());

        // elements ']'
        auto result = parseElements();
        if (result.isRight()) return result;
        if (!cursor.tryEatChar(']'))
            return Right(JsonParseError("expected ']'"_sv));

        return result;
    }

    /*
    bool isGood(char c, char d) {
        return (c != '"' && d != '"') && (c != '\\' && d != '\\') && (c >= 0x20
    && d >= 0x20);
    }
    */

    bool isGood(char c) { return c != '"' && c != '\\' && c >= 0x20; }

    /*
     * string:
     *   '"' characters '"'
     * character
     *   '0020' . '10FFFF' - '"' - '\'
     *   '\' escape
     *
     * escape:
     *   '"'
     *   '\'
     *   '/'
     *   'b'
     *   'f'
     *   'n'
     *   'r'
     *   't'
     *   'u' hex hex hex hex
     *
     * hex:
     *   digit
     *   'A' . 'F'
     *   'a' . 'f'
     */
    Either<JsonString, JsonParseError> JsonParser::parseString() {
        spargel_trace_scope("parseString");

        // '"'
        if (!cursor.tryEatChar('"'))
            return Right(JsonParseError("expected '\"'"_sv));

        // characters
        base::vector<char> chars;

        while (!cursor.isEnd()) {
            char ch = (char)cursor.consumeChar();

            if (isGood(ch)) {
                chars.emplace(ch);
                continue;
            }

            // '"'
            if (ch == '"') {
                return Left(JsonString(
                    base::string_from_range(chars.begin(), chars.end())));
            }

            // '\'
            if (ch == '\\') {
                if (cursor.isEnd())
                    return Right(JsonParseError("unfinished escape"_sv));

                ch = (char)cursor.consumeChar();
                switch (ch) {
                case '\"':
                case '\\':
                case '/':  // why escape this?
                    chars.emplace(ch);
                    break;
                case 'b':
                    chars.emplace('\b');
                    break;
                case 'f':
                    chars.emplace('\f');
                    break;
                case 'n':
                    chars.emplace('\n');
                    break;
                case 'r':
                    chars.emplace('\r');
                    break;
                case 't':
                    chars.emplace('\t');
                    break;
                case 'u': {
                    // TODO: unicode
                    u16 code = 0;
                    for (int i = 0; i < 4; i++) {
                        if (cursor.isEnd())
                            return Right(
                                JsonParseError("expected a hex digit"_sv));
                        ch = (char)cursor.consumeChar();
                        char v;
                        if ('0' <= ch && ch <= '9')
                            v = ch - '0';
                        else if ('A' <= ch && ch <= 'F')
                            v = ch - 'A' + 0xa;
                        else if ('a' <= ch && ch <= 'f')
                            v = ch - 'a' + 0xa;
                        else
                            return Right(JsonParseError("bad hex digit"_sv));
                        code = (u16)(code * 0x10 + v);
                    }
                    appendUtf8(chars, code);
                } break;
                default:
                    return Right(JsonParseError(
                        String("unexpected escape character: '") + ch + '\''));
                }
            } else if ((u8)ch >= 0x20) {
                // TODO: unicode
                // no problem for UTF-8
                chars.emplace(ch);
            } else {
                return Right(JsonParseError(String("invalid character 0x") +
                                            char2hex(ch)));
            }
        }

        return Right(JsonParseError(UNEXPECTED_END));
    }

    /*
     * integer:
     *   digit
     *   onenine digits
     *   '-' digit
     *   '-' onenine digits
     *
     * digits:
     *   digit
     *   digit digits
     *
     * digit:
     *   '0'
     *   onenine
     *
     * onenine:
     *   '1' . '9'
     */
    Optional<JsonParseError> JsonParser::parseInteger(JsonNumber& number,
                                                      bool& minus) {
        spargel_trace_scope("parseInteger");

        if (cursor.isEnd()) return makeOptional<JsonParseError>(UNEXPECTED_END);

        minus = false;
        char ch = (char)cursor.peek();

        // '-'
        if (ch == '-') {
            minus = true;
            cursor.advance();
            ch = (char)cursor.peek();
        }

        // digit
        if (ch == '0') {
            cursor.advance();
            return nullopt;
        }

        // digit
        // onenine digits
        if ('0' < ch && ch <= '9') {
            number = (ch - '0');
            cursor.advance();
            while (!cursor.isEnd()) {
                ch = (char)cursor.peek();
                if ('0' <= ch && ch <= '9') {
                    number = number * 10 + (ch - '0');
                    cursor.advance();
                } else {
                    break;
                }
            }

            return nullopt;
        }

        return makeOptional<JsonParseError>("expected a number"_sv);
    }

    /*
     * fraction:
     *   ""
     *   '.' digits
     */
    Optional<JsonParseError> JsonParser::parseFraction(JsonNumber& number) {
        spargel_trace_scope("parseFraction");

        if (cursor.isEnd()) return nullopt;

        // '.'
        if (cursor.peek() != '.') return nullopt;
        cursor.advance();

        // digits
        char ch = (char)cursor.peek();
        if (!('0' <= ch && ch <= '9'))
            return makeOptional<JsonParseError>("expected fractional part"_sv);
        JsonNumber x = 0.1;
        while (!cursor.isEnd()) {
            ch = (char)cursor.peek();
            if ('0' <= ch && ch <= '9') {
                number += x * (ch - '0');
                x *= 0.1;
                cursor.advance();
            } else {
                break;
            }
        }

        return nullopt;
    }

    /*
     * exponent:
     *   ""
     *   'E' sign digits
     *   'e' sign digits
     *
     * sign:
     *   ""
     *   '+'
     *   '-'
     */
    Optional<JsonParseError> JsonParser::parseExponent(JsonNumber& number) {
        spargel_trace_scope("parseExponent");

        if (!cursor.tryEatChar('E') && !cursor.tryEatChar('e')) return nullopt;

        bool minus = false;
        if (cursor.tryEatChar('-')) {
            minus = true;
        } else if (cursor.tryEatChar('+')) {
            // skip
        }

        // digits
        char ch = (char)cursor.peek();
        if (!('0' <= ch && ch <= '9'))
            return makeOptional<JsonParseError>("expected exponential part"_sv);
        JsonNumber exponent = 0;
        while (!cursor.isEnd()) {
            ch = (char)cursor.peek();
            if ('0' <= ch && ch <= '9') {
                exponent = exponent * 10 + (ch - '0');
                cursor.advance();
            } else {
                break;
            }
        }

        if (minus)
            number *= pow(0.1, exponent);
        else
            number *= pow(10, exponent);

        return nullopt;
    }

    /*
     * number:
     *   integer fraction exponent
     */
    Either<JsonNumber, JsonParseError> JsonParser::parseNumber() {
        spargel_trace_scope("parseNumber");

        JsonNumber number(0);
        bool minus;
        auto result = parseInteger(number, minus);
        if (result.hasValue()) return Right(base::move(result.value()));

        result = parseFraction(number);
        if (result.hasValue()) return Right(base::move(result.value()));

        result = parseExponent(number);
        if (result.hasValue()) return Right(base::move(result.value()));

        if (minus) number = -number;

        return Left(base::move(number));
    }

    /*
     *   "true"
     *   "false"
     */
    Either<JsonBoolean, JsonParseError> JsonParser::parseBoolean() {
        spargel_trace_scope("parseBoolean");

        if (cursor.tryEatString("true")) {
            return Left(JsonBoolean(true));
        } else if (cursor.tryEatString("false")) {
            return Left(JsonBoolean(false));
        } else {
            return Right(JsonParseError("expected \"true\" or \"false\""_sv));
        }
    }

    /*
     *   "null"
     */
    Optional<JsonParseError> JsonParser::parseNull() {
        spargel_trace_scope("parseNull");

        if (cursor.tryEatString("null"))
            return nullopt;
        else
            return makeOptional<JsonParseError>("expected \"null\""_sv);
    }

    /*
     * members:
     *   member
     *   member ',' members
     */
    Either<JsonObject, JsonParseError> JsonParser::parseMembers() {
        spargel_trace_scope("parseMembers");

        base::HashMap<JsonString, JsonValue> members;
        while (!cursor.isEnd()) {
            // member
            JsonString key;
            JsonValue value;
            auto result = parseMember(key, value);
            if (result.hasValue()) return Right(base::move(result.value()));

            members.set(base::move(key), base::move(value));

            // ','
            if (!cursor.tryEatChar(','))
                return Left(JsonObject(base::move(members)));
        }

        return Right(JsonParseError(UNEXPECTED_END));
    }

    /*
     * member:
     *   ws string ws ':' element
     */
    Optional<JsonParseError> JsonParser::parseMember(JsonString& key,
                                                     JsonValue& value) {
        spargel_trace_scope("parseMember");

        // ws
        eatWhitespaces();

        // string
        auto result_key = parseString();
        if (result_key.isRight())
            return makeOptional<JsonParseError>(base::move(result_key.right()));
        key = base::move(result_key.left());

        // ws
        eatWhitespaces();

        // ':'
        if (!cursor.tryEatChar(':'))
            return makeOptional<JsonParseError>("expected ':'"_sv);

        // element
        auto result_value = parseElement();
        if (result_value.isRight())
            return makeOptional<JsonParseError>(
                base::move(result_value.right()));
        value = base::move(result_value.left());

        return nullopt;
    }

    /*
     * elements:
     *   element
     *   element ',' elements
     */
    Either<JsonArray, JsonParseError> JsonParser::parseElements() {
        spargel_trace_scope("parseElements");

        base::vector<JsonValue> elements;
        while (!cursor.isEnd()) {
            // element
            auto result = parseElement();
            if (result.isRight()) return Right(base::move(result.right()));

            elements.emplace(base::move(result.left()));

            // ','
            if (!cursor.tryEatChar(','))
                return Left(JsonArray(base::move(elements)));
        }

        return Right(JsonParseError(UNEXPECTED_END));
    }

    /*
     * element:
     *   ws value ws
     */
    Either<JsonValue, JsonParseError> JsonParser::parseElement() {
        spargel_trace_scope("parseElement");

        // ws
        eatWhitespaces();

        // value
        auto result = parseValue();
        if (result.isRight()) return result;

        // ws
        eatWhitespaces();

        return result;
    }

    Either<JsonValue, JsonParseError> JsonParser::parse(const char* str,
                                                        usize length) {
        spargel_trace_scope("parseJson");

        JsonParser parser{JsonParser::Cursor{str, str + length}};
        return parser.parseElement();
    }

    bool JsonParser::Cursor::tryEatChar(char ch) {
        if (cur + 1 > end) return false;
        if (peek() != ch) return false;
        advance();
        return true;
    }

    bool JsonParser::Cursor::tryEatString(const char* str) {
        usize len = (usize)strlen(str);
        return tryEatBytes((u8*)str, len);
    }

    bool JsonParser::Cursor::tryEatBytes(const u8* bytes, usize len) {
        if (cur + len > end) return false;
        if (memcmp(bytes, cur, len) != 0) return false;
        cur += len;
        return true;
    }

}  // namespace spargel::json
