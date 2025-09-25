#pragma once

#include "spargel/codec/codec.h"
#include "spargel/json/json_value.h"

namespace spargel::codec {

    /*
     * Codec Backend
     *
     * Note: due to floating point errors, JSON backend cannot properly handle
     * large integers (U64/I64). For large integers, use strings to represent
     * them.
     */

    using JsonEncodeError = CodecError;
    using JsonDecodeError = CodecError;

    // JSON encode backend
    struct JsonEncodeBackend {
        using DataType = json::JsonValue;
        using ErrorType = JsonEncodeError;

        base::Either<json::JsonValue, JsonEncodeError> makeNull() {
            return base::Left(json::JsonValue(json::JsonNull()));
        }

        base::Either<json::JsonValue, JsonEncodeError> makeBoolean(bool b) {
            return base::Left(json::JsonValue(json::JsonBoolean(b)));
        }

        base::Either<json::JsonValue, JsonEncodeError> makeU8(u8 n) {
            return base::Left(json::JsonValue(json::JsonNumber(n)));
        }
        base::Either<json::JsonValue, JsonEncodeError> makeI8(i8 n) {
            return base::Left(json::JsonValue(json::JsonNumber(n)));
        }
        base::Either<json::JsonValue, JsonEncodeError> makeU16(u16 n) {
            return base::Left(json::JsonValue(json::JsonNumber(n)));
        }
        base::Either<json::JsonValue, JsonEncodeError> makeI16(i16 n) {
            return base::Left(json::JsonValue(json::JsonNumber(n)));
        }
        base::Either<json::JsonValue, JsonEncodeError> makeU32(u32 n) {
            return base::Left(json::JsonValue(json::JsonNumber(n)));
        }
        base::Either<json::JsonValue, JsonEncodeError> makeI32(i32 n) {
            return base::Left(json::JsonValue(json::JsonNumber(n)));
        }
        base::Either<json::JsonValue, JsonEncodeError> makeU64(u64 n) {
            return base::Left(json::JsonValue(json::JsonNumber(n)));
        }
        base::Either<json::JsonValue, JsonEncodeError> makeI64(i64 n) {
            return base::Left(json::JsonValue(json::JsonNumber(n)));
        }

        base::Either<json::JsonValue, JsonEncodeError> makeF32(f32 v) {
            return base::Left(json::JsonValue(json::JsonNumber(v)));
        }
        base::Either<json::JsonValue, JsonEncodeError> makeF64(f64 v) {
            return base::Left(json::JsonValue(json::JsonNumber(v)));
        }

        base::Either<json::JsonValue, JsonEncodeError> makeString(
            const base::String& s) {
            return base::Left(json::JsonValue(json::JsonString(s)));
        }

        base::Either<json::JsonValue, JsonEncodeError> makeArray(
            const base::Vector<json::JsonValue>& array) {
            return base::Left(json::JsonValue(json::JsonArray(array)));
        }

        base::Either<json::JsonValue, JsonEncodeError> makeMap(
            const base::HashMap<base::String, json::JsonValue>& map) {
            return base::Left(json::JsonValue(json::JsonObject(map)));
        }
    };

    // JSON decode backend
    struct JsonDecodeBackend {
        using DataType = json::JsonValue;
        using ErrorType = JsonDecodeError;

        base::Optional<JsonDecodeError> getNull(const json::JsonValue& data);

        base::Either<bool, JsonDecodeError> getBoolean(
            const json::JsonValue& data);

        base::Either<u8, JsonDecodeError> getU8(const json::JsonValue& data);
        base::Either<i8, JsonDecodeError> getI8(const json::JsonValue& data);
        base::Either<u16, JsonDecodeError> getU16(const json::JsonValue& data);
        base::Either<i16, JsonDecodeError> getI16(const json::JsonValue& data);
        base::Either<u32, JsonDecodeError> getU32(const json::JsonValue& data);
        base::Either<i32, JsonDecodeError> getI32(const json::JsonValue& data);
        base::Either<u64, JsonDecodeError> getU64(const json::JsonValue& data);
        base::Either<i64, JsonDecodeError> getI64(const json::JsonValue& data);

        base::Either<f32, JsonDecodeError> getF32(const json::JsonValue& data);
        base::Either<f64, JsonDecodeError> getF64(const json::JsonValue& data);

        base::Either<base::String, JsonDecodeError> getString(
            const json::JsonValue& data);

        base::Either<base::Vector<json::JsonValue>, JsonDecodeError> getArray(
            const json::JsonValue& data);

        base::Either<base::Optional<json::JsonValue>, JsonDecodeError>
        getMember(const json::JsonValue& data, base::StringView key);
    };

    struct JsonCodecBackend {
        using EncodeBackendType = JsonEncodeBackend;
        using DecodeBackendType = JsonDecodeBackend;
    };

}  // namespace spargel::codec
