#include "spargel/codec/model/gltf.h"

#include "spargel/base/functional.h"
#include "spargel/base/string_view.h"
#include "spargel/codec/codec.h"
#include "spargel/codec/json_codec.h"
#include "spargel/json/json_parser.h"

using namespace spargel::base::literals;

namespace spargel::codec::model {

    namespace {

        struct Vector3fDecoder {
            using TargetType = Vector3f;

            template <DecodeBackend DB>
            base::Either<Vector3f, ErrorType<DB>> decode(
                DB& backend, const DataType<DB>& data) {
                auto result =
                    makeVectorDecoder(F32Codec{}).decode(backend, data);
                if (result.isLeft()) {
                    auto array = result.left();
                    if (array.count() == 3) {
                        Vector3f v;
                        v.x = array[0];
                        v.y = array[1];
                        v.z = array[2];
                        return base::Left(base::move(v));
                    } else {
                        return base::Right<ErrorType<DB>>(
                            "Vector3f expected 3 numbers"_sv);
                    }
                } else {
                    return base::Right(base::move(result.right()));
                }
            }
        };
        static_assert(Decoder<Vector3fDecoder>);

        struct Vector4fDecoder {
            using TargetType = Vector4f;

            template <DecodeBackend DB>
            base::Either<Vector4f, ErrorType<DB>> decode(
                DB& backend, const DataType<DB>& data) {
                auto result =
                    makeVectorDecoder(F64Codec{}).decode(backend, data);
                if (result.isLeft()) {
                    auto array = result.left();
                    if (array.count() == 4) {
                        Vector4f v;
                        v.x = (float)array[0];
                        v.y = (float)array[1];
                        v.z = (float)array[2];
                        v.w = (float)array[3];
                        return base::Left(base::move(v));
                    } else {
                        return base::Right<ErrorType<DB>>(
                            "Vector4f expected 4 numbers"_sv);
                    }
                } else {
                    return base::Right(base::move(result.right()));
                }
            }
        };
        static_assert(Decoder<Vector4fDecoder>);

        struct Matrix4x4fDecoder {
            using TargetType = Matrix4x4f;

            template <DecodeBackend DB>
            static base::Either<Matrix4x4f, ErrorType<DB>> decode(
                DB& backend, const DataType<DB>& data) {
                auto result =
                    makeVectorDecoder(F32Codec{}).decode(backend, data);
                if (result.isLeft()) {
                    auto array = result.left();
                    if (array.count() == 16) {
                        Matrix4x4f mat;
                        for (u8 i = 0; i < 16; i++) {
                            mat.entries[i] = array[i];
                        }
                        return base::Left(base::move(mat));
                    } else {
                        return base::Right<ErrorType<DB>>(
                            "Matrix4x4f expected 16 numbers"_sv);
                    }
                } else {
                    return base::Right(base::move(result.right()));
                }
            }
        };
        static_assert(Decoder<Matrix4x4fDecoder>);

        auto glTFAccessorDecoder = makeRecordDecoder<GlTFAccessor>(
            base::Constructor<GlTFAccessor>{},
            makeOptionalDecodeField("bufferView"_sv, I32Codec{}),
            makeOptionalDecodeField("byteOffset"_sv, I32Codec{}),
            makeNormalDecodeField("componentType"_sv, I32Codec{}),
            makeOptionalDecodeField("normalized"_sv, BooleanCodec{}),
            makeNormalDecodeField("count"_sv, I32Codec{}),
            makeNormalDecodeField("type"_sv, StringCodec{}),
            makeOptionalDecodeField("max"_sv, makeVectorDecoder(F64Codec{})),
            makeOptionalDecodeField("min"_sv, makeVectorDecoder(F64Codec{})),
            makeOptionalDecodeField("name"_sv, StringCodec{}));

        auto glTFAssetDecoder = makeRecordDecoder<GlTFAsset>(
            base::Constructor<GlTFAsset>{},
            makeOptionalDecodeField("copyright"_sv, StringCodec{}),
            makeOptionalDecodeField("generator"_sv, StringCodec{}),
            makeNormalDecodeField("version"_sv, StringCodec{}),
            makeOptionalDecodeField("minVersion"_sv, StringCodec{}));

        auto glTFBufferDecoder = makeRecordDecoder<GlTFBuffer>(
            base::Constructor<GlTFBuffer>{},
            makeOptionalDecodeField("uri"_sv, StringCodec{}),
            makeNormalDecodeField("byteLength"_sv, I32Codec{}),
            makeOptionalDecodeField("name"_sv, StringCodec{}));

        auto glTFBufferViewDecoder = makeRecordDecoder<GlTFBufferView>(
            base::Constructor<GlTFBufferView>{},
            makeNormalDecodeField("buffer"_sv, I32Codec{}),
            makeOptionalDecodeField("byteOffset"_sv, I32Codec{}),
            makeNormalDecodeField("byteLength"_sv, I32Codec{}),
            makeOptionalDecodeField("byteStride"_sv, I32Codec{}),
            makeOptionalDecodeField("target"_sv, I32Codec{}),
            makeOptionalDecodeField("name"_sv, StringCodec{}));

        auto glTFMeshPrimitiveAttributesDecoder =
            makeRecordDecoder<GlTFMeshPrimitiveAttributes>(
                base::Constructor<GlTFMeshPrimitiveAttributes>{},
                makeOptionalDecodeField("POSITION"_sv, I32Codec{}),
                makeOptionalDecodeField("NORMAL"_sv, I32Codec{}),
                makeOptionalDecodeField("TEXCOORD_0"_sv, I32Codec{}),
                makeOptionalDecodeField("COLOR_0"_sv, I32Codec{}));

        auto glTFMeshPrimitiveDecoder = makeRecordDecoder<GlTFMeshPrimitive>(
            base::Constructor<GlTFMeshPrimitive>{},
            makeNormalDecodeField("attributes"_sv,
                                  glTFMeshPrimitiveAttributesDecoder),
            makeOptionalDecodeField("indices"_sv, I32Codec{}),
            makeOptionalDecodeField("material"_sv, I32Codec{}),
            makeOptionalDecodeField("mode"_sv, I32Codec{}));

        auto glTFMeshDecoder = makeRecordDecoder<GlTFMesh>(
            base::Constructor<GlTFMesh>{},
            makeNormalDecodeField("primitives"_sv,
                                  makeVectorDecoder(glTFMeshPrimitiveDecoder)),
            makeOptionalDecodeField("weights"_sv,
                                    makeVectorDecoder(F64Codec{})),
            makeOptionalDecodeField("name"_sv, StringCodec{}));

        auto glTFNodeDecoder = makeRecordDecoder<GlTFNode>(
            base::Constructor<GlTFNode>{},
            makeOptionalDecodeField("camera"_sv, I32Codec{}),
            makeOptionalDecodeField("children"_sv,
                                    makeVectorDecoder(I32Codec{})),
            makeOptionalDecodeField("skin"_sv, I32Codec{}),
            makeOptionalDecodeField("matrix"_sv, Matrix4x4fDecoder{}),
            makeOptionalDecodeField("mesh"_sv, I32Codec{}),
            makeOptionalDecodeField("rotation"_sv, Vector4fDecoder{}),
            makeOptionalDecodeField("scale"_sv, Vector3fDecoder{}),
            makeOptionalDecodeField("translation"_sv, Vector3fDecoder{}),
            makeOptionalDecodeField("weights"_sv,
                                    makeVectorDecoder(F64Codec{})),
            makeOptionalDecodeField("name"_sv, StringCodec{}));

        auto glTFSceneDecoder = makeRecordDecoder<GlTFScene>(
            base::Constructor<GlTFScene>{},
            makeOptionalDecodeField("nodes"_sv, makeVectorDecoder(I32Codec{})),
            makeOptionalDecodeField("name"_sv, StringCodec{}));

        auto glTFDecoder = makeRecordDecoder<GlTF>(
            base::Constructor<GlTF>{},
            makeOptionalDecodeField("accessors"_sv,
                                    makeVectorDecoder(glTFAccessorDecoder)),
            makeNormalDecodeField("asset"_sv, glTFAssetDecoder),
            makeOptionalDecodeField("buffers"_sv,
                                    makeVectorDecoder(glTFBufferDecoder)),
            makeOptionalDecodeField("bufferViews"_sv,
                                    makeVectorDecoder(glTFBufferViewDecoder)),
            makeOptionalDecodeField("meshes"_sv,
                                    makeVectorDecoder(glTFMeshDecoder)),
            makeOptionalDecodeField("nodes"_sv,
                                    makeVectorDecoder(glTFNodeDecoder)),
            makeOptionalDecodeField("scene"_sv, I32Codec{}),
            makeOptionalDecodeField("scenes"_sv,
                                    makeVectorDecoder(glTFSceneDecoder)));

    }  // namespace

    base::Either<GlTF, GlTFDecodeError> parseGlTF(const char* text, usize len) {
        auto json_result = json::JsonParser::parse(text, len);
        if (json_result.isRight())
            return base::Right<GlTFDecodeError>(json_result.right().message());

        JsonDecodeBackend backend;
        auto result =
            glTFDecoder.decode(backend, base::move(json_result.left()));
        if (result.isLeft())
            return base::Left(base::move(result.left()));
        else
            return base::Right<GlTFDecodeError>(
                base::move(result.right().message()));
    }

}  // namespace spargel::codec::model
