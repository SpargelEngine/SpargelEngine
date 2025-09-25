#include "spargel/base/check.h"
#include "spargel/base/test.h"
#include "spargel/json/json_parser.h"
#include "spargel/json/json_value.h"

// libc
#include <string.h>

// libm
#include <math.h>

using namespace spargel;
using namespace spargel::json;

namespace {

    base::Either<JsonValue, JsonParseError> parseJson(const char* str) {
        return JsonParser::parse(str, strlen(str));
    }

    TEST(JSON_JsonValue) {
        JsonValue v = JsonObject();
        v.object.members.set(JsonString("error"), JsonNull());
        v.object.members.set(JsonString("active"), JsonBoolean());

        spargel_check(v.type == JsonValueType::object);
        spargel_check(v.object.members.count() == 2);

        JsonValue v2 = v;
        v2 = JsonValue(JsonArray());
        v2.array.elements.emplace(JsonNumber(123.456));
        v2.array.elements.emplace(JsonString("hello"));

        spargel_check(v2.type == JsonValueType::array);
        spargel_check(v2.array.elements.count() == 2);

        spargel_check(v.type == JsonValueType::object);
        spargel_check(v.object.members.count() == 2);

        v = base::move(v2);
        spargel_check(v.type == JsonValueType::array);
        spargel_check(v.array.elements.count() == 2);

        JsonValue v3 = base::move(v);
        spargel_check(v3.type == JsonValueType::array);
        spargel_check(v3.array.elements.count() == 2);
    }

    TEST(JSON_Parse_Primitive) {
        base::Either<JsonValue, JsonParseError> result =
            base::Left(JsonValue());

        result = parseJson("null");
        spargel_check(result.isLeft() && result.left() == JsonNull());

        result = parseJson("true");
        spargel_check(result.isLeft() && result.left() == JsonBoolean(true));

        result = parseJson("false");
        spargel_check(result.isLeft() && result.left() == JsonBoolean(false));

        result = parseJson("\"string\"");
        spargel_check(result.isLeft() && result.left() == JsonString("string"));

        result = parseJson("\"123\\n\\\"xyz\"");
        spargel_check(result.isLeft() &&
                      result.left() == JsonString("123\n\"xyz"));

        result = parseJson("\" \\\" \\\\ \\/ \\b \\f \\n \\r \\t \"");
        spargel_check(result.isLeft() &&
                      result.left() == JsonString(" \" \\ / \b \f \n \r \t "));

        result = parseJson("\"\\u0020\\u0041\\u0061\"");
        spargel_check(result.isLeft() && result.left() == JsonString(" Aa"));

        result = parseJson("\"\\u4e2d\\u6587\"");
        spargel_check(result.isLeft() && result.left() == JsonString("中文"));

        result = parseJson("\"\\u65e5\\u672c\\u8a9e\"");
        spargel_check(result.isLeft() && result.left() == JsonString("日本語"));

        result = parseJson("\"\\ud55c\\uad6d\\uc5b4\"");
        spargel_check(result.isLeft() && result.left() == JsonString("한국어"));

        result = parseJson("\"\n\"");
        spargel_check(result.isRight());

        result = parseJson("0");
        spargel_check(result.isLeft() && result.left() == JsonNumber(0));

        result = parseJson("12345");
        spargel_check(result.isLeft() && result.left() == JsonNumber(12345));

        result = parseJson("-54321");
        spargel_check(result.isLeft() && result.left() == JsonNumber(-54321));

        result = parseJson("123.000456");
        spargel_check(result.isLeft() &&
                      result.left() == JsonNumber(123.000456));

        result = parseJson("-654.000321");
        spargel_check(result.isLeft() &&
                      result.left() == JsonNumber(-654.000321));

        result = parseJson("0.001");
        spargel_check(result.isLeft() && result.left() == JsonNumber(0.001));

        result = parseJson("-0.001");
        spargel_check(result.isLeft() && result.left() == JsonNumber(-0.001));

        result = parseJson("0.001e4");
        spargel_check(result.isLeft() && result.left() == JsonNumber(0.001e4));

        result = parseJson("-0.001e+3");
        spargel_check(result.isLeft() &&
                      result.left() == JsonNumber(-0.001e+3));

        result = parseJson("-0.001E-3");
        spargel_check(result.isLeft() &&
                      result.left() == JsonNumber(-0.001E-3));
    }

    TEST(JSON_Parse_Array) {
        base::Either<JsonValue, JsonParseError> result =
            base::Left(JsonValue());

        result = parseJson("[]");
        spargel_check(result.isLeft() &&
                      result.left().type == JsonValueType::array);
        spargel_check(result.left().array.elements.count() == 0);

        result = parseJson("[ true, \"ABC\", -123.456, null, false ]");
        spargel_check(result.isLeft() &&
                      result.left().type == JsonValueType::array);
        auto array = result.left().array;
        spargel_check(array.elements.count() == 5);
        spargel_check(array.elements[0] == JsonBoolean(true));
        spargel_check(array.elements[1] == JsonString("ABC"));
        spargel_check(array.elements[2] == JsonNumber(-123.456));
        spargel_check(array.elements[3] == JsonNull());
        spargel_check(array.elements[4] == JsonBoolean(false));
    }

    TEST(JSON_Parse_Object) {
        base::Either<JsonValue, JsonParseError> result =
            base::Left(JsonValue());

        result = parseJson("{}");
        spargel_check(result.isLeft() &&
                      result.left().type == JsonValueType::object);
        spargel_check(result.left().object.members.count() == 0);

        result = parseJson(
            "{ \"error\": null, \"name\": \"Alice\", \"age\": 20, \"happy\": "
            "true }");

        spargel_check(result.isLeft() &&
                      result.left().type == JsonValueType::object);
        auto object = result.left().object;
        spargel_check(object.members.count() == 4);
        spargel_check(isMemberEqual(object, JsonString("error"), JsonNull()));
        spargel_check(
            isMemberEqual(object, JsonString("name"), JsonString("Alice")));
        spargel_check(isMemberEqual(object, JsonString("age"), JsonNumber(20)));
        spargel_check(
            isMemberEqual(object, JsonString("happy"), JsonBoolean(true)));
    }

    TEST(JSON_Parse_Composite) {
        base::Either<JsonValue, JsonParseError> result =
            base::Left(JsonValue());
        const char* str;

        str =
            "{\n"
            "   \"model\": \"deepseek-chat\",\n"
            "   \"choices\": [\n"
            "       {\n"
            "           \"index\": 0,\n"
            "           \"role\": \"assistant\",\n"
            "           \"logprobs\":null,\n"
            "           \"finish_reason\":\"stop\"\n"
            "       }\n"
            "   ],\n"
            "   \"usage\": {\n"
            "       \"prompt_tokens\":11,\n"
            "       \"completion_tokens\":11,\n"
            "       \"total_tokens\":22\n"
            "   }\n"
            "}";
        result = parseJson(str);
        spargel_check(result.isLeft());
        auto value = result.left();

        spargel_check(value.type == JsonValueType::object);
        spargel_check(value.object.members.count() == 3);

        spargel_check(isMemberEqual(value.object, JsonString("model"),
                                    JsonString("deepseek-chat")));

        auto* ptr1 = value.object.members.get(JsonString("choices"));
        spargel_check(ptr1 != nullptr);
        auto& v1 = *ptr1;
        spargel_check(v1.type == JsonValueType::array);
        spargel_check(v1.array.elements.count() == 1);
        auto& v11 = v1.array.elements[0];
        spargel_check(v11.type == JsonValueType::object);
        spargel_check(
            isMemberEqual(v11.object, JsonString("index"), JsonNumber(0)));
        spargel_check(isMemberEqual(v11.object, JsonString("role"),
                                    JsonString("assistant")));
        spargel_check(
            isMemberEqual(v11.object, JsonString("logprobs"), JsonNull()));
        spargel_check(isMemberEqual(v11.object, JsonString("finish_reason"),
                                    JsonString("stop")));

        auto* ptr2 = value.object.members.get(JsonString("usage"));
        spargel_check(ptr2 != nullptr);
        auto& v2 = *ptr2;
        spargel_check(v2.type == JsonValueType::object);
        spargel_check(isMemberEqual(v2.object, JsonString("prompt_tokens"),
                                    JsonNumber(11)));
        spargel_check(isMemberEqual(v2.object, JsonString("completion_tokens"),
                                    JsonNumber(11)));
        spargel_check(isMemberEqual(v2.object, JsonString("total_tokens"),
                                    JsonNumber(22)));
    }

}  // namespace
