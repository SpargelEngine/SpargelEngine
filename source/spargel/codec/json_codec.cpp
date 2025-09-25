#include "spargel/codec/json_codec.h"

#include "spargel/base/string_view.h"

namespace spargel::codec {

    using namespace base::literals;

    static_assert(EncodeBackend<JsonEncodeBackend>);
    static_assert(DecodeBackend<JsonDecodeBackend>);

    base::Optional<JsonDecodeError> JsonDecodeBackend::getNull(
        const json::JsonValue& data) {
        if (data.type != json::JsonValueType::null) {
            return base::makeOptional<JsonDecodeError>("expected null"_sv);
        }
        return base::nullopt;
    }

    base::Either<bool, JsonDecodeError> JsonDecodeBackend::getBoolean(
        const json::JsonValue& data) {
        if (data.type != json::JsonValueType::boolean)
            return base::Right(
                JsonDecodeError("expected a boolean (true/false)"_sv));
        else
            return base::Left(data.boolean);
    }

    inline constexpr auto EXPECTED_NUMBER = "expected a number"_sv;

    base::Either<u8, JsonDecodeError> JsonDecodeBackend::getU8(
        const json::JsonValue& data) {
        if (data.type == json::JsonValueType::number) {
            return base::Left(static_cast<u8>(data.number));
        } else {
            return base::Right(JsonDecodeError(EXPECTED_NUMBER));
        }
    }
    base::Either<i8, JsonDecodeError> JsonDecodeBackend::getI8(
        const json::JsonValue& data) {
        if (data.type == json::JsonValueType::number) {
            return base::Left(static_cast<i8>(data.number));
        } else {
            return base::Right(JsonDecodeError(EXPECTED_NUMBER));
        }
    }
    base::Either<u16, JsonDecodeError> JsonDecodeBackend::getU16(
        const json::JsonValue& data) {
        if (data.type == json::JsonValueType::number) {
            return base::Left(static_cast<u16>(data.number));
        } else {
            return base::Right(JsonDecodeError(EXPECTED_NUMBER));
        }
    }
    base::Either<i16, JsonDecodeError> JsonDecodeBackend::getI16(
        const json::JsonValue& data) {
        if (data.type == json::JsonValueType::number) {
            return base::Left(static_cast<i16>(data.number));
        } else {
            return base::Right(JsonDecodeError(EXPECTED_NUMBER));
        }
    }
    base::Either<u32, JsonDecodeError> JsonDecodeBackend::getU32(
        const json::JsonValue& data) {
        if (data.type == json::JsonValueType::number) {
            return base::Left(static_cast<u32>(data.number));
        } else {
            return base::Right(JsonDecodeError(EXPECTED_NUMBER));
        }
    }
    base::Either<i32, JsonDecodeError> JsonDecodeBackend::getI32(
        const json::JsonValue& data) {
        if (data.type == json::JsonValueType::number) {
            return base::Left(static_cast<i32>(data.number));
        } else {
            return base::Right(JsonDecodeError(EXPECTED_NUMBER));
        }
    }
    base::Either<u64, JsonDecodeError> JsonDecodeBackend::getU64(
        const json::JsonValue& data) {
        if (data.type == json::JsonValueType::number) {
            return base::Left(static_cast<u64>(data.number));
        } else {
            return base::Right(JsonDecodeError(EXPECTED_NUMBER));
        }
    }
    base::Either<i64, JsonDecodeError> JsonDecodeBackend::getI64(
        const json::JsonValue& data) {
        if (data.type == json::JsonValueType::number) {
            return base::Left(static_cast<i64>(data.number));
        } else {
            return base::Right(JsonDecodeError(EXPECTED_NUMBER));
        }
    }

    base::Either<f32, JsonDecodeError> JsonDecodeBackend::getF32(
        const json::JsonValue& data) {
        if (data.type == json::JsonValueType::number) {
            return base::Left(static_cast<f32>(data.number));
        } else {
            return base::Right(JsonDecodeError(EXPECTED_NUMBER));
        }
    }
    base::Either<f64, JsonDecodeError> JsonDecodeBackend::getF64(
        const json::JsonValue& data) {
        if (data.type == json::JsonValueType::number) {
            return base::Left(static_cast<f64>(data.number));
        } else {
            return base::Right(JsonDecodeError(EXPECTED_NUMBER));
        }
    }

    base::Either<base::String, JsonDecodeError> JsonDecodeBackend::getString(
        const json::JsonValue& data) {
        if (data.type == json::JsonValueType::string) {
            return base::Left(data.string);
        } else {
            return base::Right(JsonDecodeError("expected a string"_sv));
        }
    }

    base::Either<base::Vector<json::JsonValue>, JsonDecodeError>
    JsonDecodeBackend::getArray(const json::JsonValue& data) {
        if (data.type == json::JsonValueType::array) {
            return base::Left(data.array.elements);
        } else {
            return base::Right(JsonDecodeError("expected an array"_sv));
        }
    }

    base::Either<base::Optional<json::JsonValue>, JsonDecodeError>
    JsonDecodeBackend::getMember(const json::JsonValue& data,
                                 base::StringView key) {
        if (data.type == json::JsonValueType::object) {
            auto* ptr = data.object.members.get(key);
            return base::Left(ptr != nullptr
                                  ? base::makeOptional<json::JsonValue>(*ptr)
                                  : base::nullopt);
        } else {
            return base::Right(JsonDecodeError("expected an object"_sv));
        }
    }

}  // namespace spargel::codec
