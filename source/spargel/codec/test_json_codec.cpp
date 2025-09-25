#include "spargel/base/functional.h"
#include "spargel/base/string_view.h"
#include "spargel/base/test.h"
#include "spargel/codec/codec.h"
#include "spargel/codec/json_codec.h"
#include "spargel/json/json_parser.h"
#include "spargel/json/json_value.h"
#include "spargel/math/function.h"
// #include "spargel/codec/test_json.h"

using namespace spargel::base::literals;

namespace spargel::codec {

    namespace {

        using namespace json;

        static_assert(CodecBackend<JsonCodecBackend>);

        struct Student {
            base::String type = base::String("normal");
            base::String name;
            base::Optional<base::String> nickname;
            u32 age;
            bool happy;
            base::vector<f32> scores;

            static auto codec() {
                return makeRecordCodec<Student>(
                    base::Constructor<Student>{},
                    makeDefaultField<Student>("type"_sv, StringCodec{},
                                              base::String("normal"),
                                              [](auto& o) { return o.type; }),
                    makeNormalField("name"_sv, StringCodec{},
                                    [](auto& o) { return o.name; }),
                    makeOptionalField<Student>(
                        "nickname"_sv, StringCodec{},
                        [](auto& o) { return o.nickname; }),
                    makeNormalField("age"_sv, U32Codec{},
                                    [](auto& o) { return o.age; }),
                    makeNormalField("happy"_sv, BooleanCodec{},
                                    [](auto& o) { return o.happy; }),
                    makeNormalField("scores"_sv, makeVectorCodec(F32Codec{}),
                                    [](auto& o) { return o.scores; }));
            }
        };

        using EB = JsonEncodeBackend;
        using DB = JsonDecodeBackend;

        auto encodeBackend = JsonEncodeBackend();
        auto decodeBackend = JsonDecodeBackend();

        auto studentCodec = Student::codec();

        TEST(JsonCodec_Encode_Error) {
            auto result = ErrorCodec<bool>("encode error"_sv, "decode_error"_sv)
                              .encode(encodeBackend, true);
            spargel_check(result.isRight());
        }

        TEST(JsonCodec_Decode_Error) {
            auto result = ErrorCodec<bool>("encode error"_sv, "decode_error"_sv)
                              .decode(decodeBackend, JsonValue());
            spargel_check(result.isRight());
        }

        TEST(JsonCodec_Encode_Primitive) {
            base::Either<JsonValue, JsonEncodeError> result =
                base::Left(JsonValue());

            result = NullCodec{}.encode(encodeBackend, nullptr);
            spargel_check(result.isLeft() && result.left() == JsonNull());

            result = BooleanCodec{}.encode(encodeBackend, true);
            spargel_check(result.isLeft() &&
                          result.left() == JsonBoolean(true));

            result = BooleanCodec{}.encode(encodeBackend, false);
            spargel_check(result.isLeft() &&
                          result.left() == JsonBoolean(false));

            result = U32Codec{}.encode(encodeBackend, 4294967295);
            spargel_check(result.isLeft() &&
                          result.left() == JsonNumber(4294967295));

            result = I32Codec{}.encode(encodeBackend, -2147483648);
            spargel_check(result.isLeft() &&
                          result.left() == JsonNumber(-2147483648));

            result = F32Codec{}.encode(encodeBackend, 123.456f);
            spargel_check(result.isLeft() &&
                          result.left() == JsonNumber(123.456f));

            result = F64Codec{}.encode(encodeBackend, 789.012);
            spargel_check(result.isLeft() &&
                          result.left() == JsonNumber(789.012));

            result = StringCodec{}.encode(encodeBackend, base::String("ABC"));
            spargel_check(result.isLeft() &&
                          result.left() == JsonString("ABC"));
        }

        TEST(JsonCodec_Decode_Primitive) {
            {
                auto result =
                    NullCodec{}.decode(decodeBackend, JsonValue(JsonNull()));
                spargel_check(result.isLeft());
            }
            {
                auto result = BooleanCodec{}.decode(
                    decodeBackend, JsonValue(JsonBoolean(true)));
                spargel_check(result.isLeft() && result.left() == true);
            }
            {
                auto result = BooleanCodec{}.decode(
                    decodeBackend, JsonValue(JsonBoolean(false)));
                spargel_check(result.isLeft() && result.left() == false);
            }
            {
                auto result = U32Codec{}.decode(
                    decodeBackend, JsonValue(JsonNumber(4294967295)));
                spargel_check(result.isLeft() && result.left() == 4294967295);
            }
            {
                auto result = I32Codec{}.decode(
                    decodeBackend, JsonValue(JsonNumber(-2147483648)));
                spargel_check(result.isLeft() && result.left() == -2147483648);
            }
            {
                auto result = F32Codec{}.decode(
                    decodeBackend, JsonValue(JsonNumber(123.456f)));
                spargel_check(result.isLeft() &&
                              math::abs(result.left() - 123.456f) < 1e-6f);
            }
            {
                auto result = F64Codec{}.decode(decodeBackend,
                                                JsonValue(JsonNumber(789.012)));
                spargel_check(result.isLeft() &&
                              fabs(result.left() - 789.012) < 1e-6f);
            }
            {
                auto result = StringCodec{}.decode(
                    decodeBackend, JsonValue(JsonString("ABC")));
                spargel_check(result.isLeft() &&
                              result.left() == base::String("ABC"));
            }
        }

        TEST(JsonCodec_Encode_Array) {
            {
                base::vector<base::String> v;
                v.emplace("ABC");
                v.emplace("123");
                v.emplace("!@#$");
                auto result = makeVectorCodec(StringCodec{})
                                  .encode(encodeBackend, base::move(v));
                spargel_check(result.isLeft() &&
                              result.left().type == JsonValueType::array);

                auto& array = result.left().array.elements;
                spargel_check(array.count() == 3);
                spargel_check(array[0] == JsonString("ABC"));
                spargel_check(array[1] == JsonString("123"));
                spargel_check(array[2] == JsonString("!@#$"));
            }
            {
                base::vector<base::vector<i32>> v;
                base::vector<i32> v1;
                v1.emplace(1);
                v1.emplace(2);
                v.emplace(base::move(v1));
                base::vector<i32> v2;
                v2.emplace(-3);
                v2.emplace(-4);
                v.emplace(base::move(v2));
                auto result = makeVectorCodec(makeVectorCodec(I32Codec{}))
                                  .encode(encodeBackend, base::move(v));
                spargel_check(result.isLeft() &&
                              result.left().type == JsonValueType::array);

                auto& array = result.left().array.elements;
                spargel_check(array.count() == 2);
                spargel_check(array[0].type == JsonValueType::array);
                spargel_check(array[1].type == JsonValueType::array);
                auto& array1 = array[0].array.elements;
                spargel_check(array1[0] == JsonNumber(1));
                spargel_check(array1[1] == JsonNumber(2));
                auto& array2 = array[1].array.elements;
                spargel_check(array2[0] == JsonNumber(-3));
                spargel_check(array2[1] == JsonNumber(-4));
            }
        }

        TEST(JsonCodec_Decode_Array) {
            {
                auto result_json = JsonParser::parse("[123, 456, 789]");
                spargel_check(result_json.isLeft());

                auto result =
                    makeVectorCodec(U32Codec{})
                        .decode(decodeBackend, base::move(result_json.left()));
                spargel_check(result.isLeft());

                auto& array = result.left();
                spargel_check(array.count() == 3);
                spargel_check(array[0] == 123);
                spargel_check(array[1] == 456);
                spargel_check(array[2] == 789);
            }
            {
                auto result_json = JsonParser::parse(
                    "[[\"ABC\", \"123\"], [\"XYZ\", \"789\"]]");
                spargel_check(result_json.isLeft());

                auto result =
                    makeVectorCodec(makeVectorCodec(StringCodec{}))
                        .decode(decodeBackend, base::move(result_json.left()));
                spargel_check(result.isLeft());

                auto& array = result.left();
                spargel_check(array[0][0] == base::String("ABC"));
                spargel_check(array[0][1] == base::String("123"));
                spargel_check(array[1][0] == base::String("XYZ"));
                spargel_check(array[1][1] == base::String("789"));
            }
        }

        TEST(JsonCodec_Encode_Record) {
            {
                Student student;
                student.name = "Alice";
                student.age = 20;
                student.happy = true;
                base::vector<f32> scores;
                scores.emplace(98.0f);
                scores.emplace(87.5f);
                scores.emplace(92.0f);
                student.scores = base::move(scores);

                auto result =
                    studentCodec.encode(encodeBackend, base::move(student));
                spargel_check(result.isLeft());

                auto& object = result.left().object;
                spargel_check(isMemberEqual(object, JsonString("type"),
                                            JsonString("normal")));
                spargel_check(isMemberEqual(object, JsonString("name"),
                                            JsonString("Alice")));
                auto* p_nickname = object.members.get(JsonString("nickname"));
                spargel_check(p_nickname == nullptr);
                spargel_check(
                    isMemberEqual(object, JsonString("age"), JsonNumber(20)));
                spargel_check(isMemberEqual(object, JsonString("happy"),
                                            JsonBoolean(true)));
                auto* p_array = object.members.get(JsonString("scores"));
                spargel_check(p_array != nullptr &&
                              p_array->type == JsonValueType::array);
                auto& array = p_array->array.elements;
                spargel_check(array.count() == 3);
                spargel_check(array[0] == JsonNumber(98));
                spargel_check(array[1] == JsonNumber(87.5));
                spargel_check(array[2] == JsonNumber(92));
            }
        }

        TEST(JsonCodec_Decode_Record) {
            {
                const auto str = R"(
                        {
                            "type": "normal",
                            "name": "Alice",
                            "age": 20,
                            "happy": true,
                            "scores": [98, 87.5, 92]
                        }
                    )";
                auto result_json = JsonParser::parse(str);
                spargel_check(result_json.isLeft());

                auto result = studentCodec.decode(
                    decodeBackend, base::move(result_json.left()));
                spargel_check(result.isLeft());

                auto& student = result.left();
                spargel_check(student.type == base::String("normal"));
                spargel_check(student.name == base::String("Alice"));
                spargel_check(!student.nickname.hasValue());
                spargel_check(student.age == 20);
                spargel_check(student.happy == true);
                spargel_check(student.scores.count() == 3);
                spargel_check(fabs(student.scores[0] - 98) < 1e-6f);
                spargel_check(fabs(student.scores[1] - 87.5f) < 1e-6f);
                spargel_check(fabs(student.scores[2] - 92) < 1e-6f);
            }
            {
                const auto str =
                    "[\n"
                    "  {\n"
                    "    \"type\": \"normal\",\n"
                    "    \"name\": \"Alice\",\n"
                    "    \"age\": 20,\n"
                    "    \"happy\": true,\n"
                    "    \"scores\": [98, 87.5, 92]\n"
                    "  },\n"
                    "  {\n"
                    "    \"name\": \"Bob\",\n"
                    "    \"nickname\": \"Bomb\",\n"
                    "    \"age\": 18,\n"
                    "    \"happy\": false,\n"
                    "    \"scores\": [65.5]\n"
                    "  },\n"
                    "  {\n"
                    "    \"type\": \"exchange\",\n"
                    "    \"name\": \"David\",\n"
                    "    \"age\": 21,\n"
                    "    \"happy\": true,\n"
                    "    \"scores\": [99, 97.5]\n"
                    "  }\n"
                    "]";
                auto result_json = JsonParser::parse(str);
                spargel_check(result_json.isLeft());

                auto result =
                    makeVectorCodec(studentCodec)
                        .decode(decodeBackend, base::move(result_json.left()));
                spargel_check(result.isLeft());

                auto& students = result.left();

                spargel_check(students[0].type == base::String("normal"));
                spargel_check(students[0].name == base::String("Alice"));
                spargel_check(!students[0].nickname.hasValue());
                spargel_check(students[0].age == 20);
                spargel_check(students[0].happy == true);
                spargel_check(students[0].scores.count() == 3);
                spargel_check(fabs(students[0].scores[0] - 98) < 1e-6f);
                spargel_check(fabs(students[0].scores[1] - 87.5f) < 1e-6f);
                spargel_check(fabs(students[0].scores[2] - 92) < 1e-6f);

                spargel_check(students[1].type == base::String("normal"));
                spargel_check(students[1].name == base::String("Bob"));
                spargel_check(students[1].nickname.hasValue() &&
                              students[1].nickname.value() ==
                                  base::String("Bomb"));
                spargel_check(students[1].age == 18);
                spargel_check(students[1].happy == false);
                spargel_check(students[1].scores.count() == 1);
                spargel_check(fabs(students[1].scores[0] - 65.5) < 1e-6f);

                spargel_check(students[2].type == base::String("exchange"));
                spargel_check(students[2].name == base::String("David"));
                spargel_check(!students[2].nickname.hasValue());
                spargel_check(students[2].age == 21);
                spargel_check(students[2].happy == true);
                spargel_check(students[2].scores.count() == 2);
                spargel_check(fabs(students[2].scores[0] - 99) < 1e-6f);
                spargel_check(fabs(students[2].scores[1] - 97.5f) < 1e-6f);
            }
        }

    }  // namespace
}  // namespace spargel::codec
