#include "spargel/gltf/gltf.h"

#include "spargel/base/functional.h"
#include "spargel/base/string_view.h"
#include "spargel/codec/codec.h"
#include "spargel/codec/json_codec.h"
#include "spargel/json/json_parser.h"

namespace spargel::gltf {

    namespace {

        using namespace base::literals;

        struct Vector3fDecoder {
            using TargetType = math::Vector3f;

            template <codec::DecodeBackend DB>
            base::Either<math::Vector3f, codec::ErrorType<DB>> decode(
                DB& backend, const codec::DataType<DB>& data) {
                auto result =
                    makeVectorDecoder(codec::F32Codec{}).decode(backend, data);
                if (result.isLeft()) {
                    auto array = result.left();
                    if (array.count() == 3) {
                        math::Vector3f v;
                        v.x = array[0];
                        v.y = array[1];
                        v.z = array[2];
                        return base::Left(base::move(v));
                    } else {
                        return base::Right<codec::ErrorType<DB>>(
                            "Vector3f expected 3 numbers"_sv);
                    }
                } else {
                    return base::Right(base::move(result.right()));
                }
            }
        };
        static_assert(codec::Decoder<Vector3fDecoder>);

        struct Vector4fDecoder {
            using TargetType = math::Vector4f;

            template <codec::DecodeBackend DB>
            base::Either<math::Vector4f, codec::ErrorType<DB>> decode(
                DB& backend, const codec::DataType<DB>& data) {
                auto result =
                    makeVectorDecoder(codec::F64Codec{}).decode(backend, data);
                if (result.isLeft()) {
                    auto array = result.left();
                    if (array.count() == 4) {
                        math::Vector4f v;
                        v.x = (float)array[0];
                        v.y = (float)array[1];
                        v.z = (float)array[2];
                        v.w = (float)array[3];
                        return base::Left(base::move(v));
                    } else {
                        return base::Right<codec::ErrorType<DB>>(
                            "Vector4f expected 4 numbers"_sv);
                    }
                } else {
                    return base::Right(base::move(result.right()));
                }
            }
        };
        static_assert(codec::Decoder<Vector4fDecoder>);

        struct Matrix4x4fDecoder {
            using TargetType = math::Matrix4x4f;

            template <codec::DecodeBackend DB>
            static base::Either<math::Matrix4x4f, codec::ErrorType<DB>> decode(
                DB& backend, const codec::DataType<DB>& data) {
                auto result =
                    makeVectorDecoder(codec::F32Codec{}).decode(backend, data);
                if (result.isLeft()) {
                    auto array = result.left();
                    if (array.count() == 16) {
                        math::Matrix4x4f mat;
                        for (u8 i = 0; i < 16; i++) {
                            mat.entries[i] = array[i];
                        }
                        return base::Left(base::move(mat));
                    } else {
                        return base::Right<codec::ErrorType<DB>>(
                            "Matrix4x4f expected 16 numbers"_sv);
                    }
                } else {
                    return base::Right(base::move(result.right()));
                }
            }
        };
        static_assert(codec::Decoder<Matrix4x4fDecoder>);

        auto glTFAccessorDecoder = makeRecordDecoder<GlTFAccessor>(
            base::Constructor<GlTFAccessor>{},
            makeOptionalDecodeField("bufferView"_sv, codec::I32Codec{}),
            makeOptionalDecodeField("byteOffset"_sv, codec::I32Codec{}),
            makeNormalDecodeField("componentType"_sv, codec::I32Codec{}),
            makeOptionalDecodeField("normalized"_sv, codec::BooleanCodec{}),
            makeNormalDecodeField("count"_sv, codec::I32Codec{}),
            makeNormalDecodeField("type"_sv, codec::StringCodec{}),
            makeOptionalDecodeField("max"_sv,
                                    makeVectorDecoder(codec::F64Codec{})),
            makeOptionalDecodeField("min"_sv,
                                    makeVectorDecoder(codec::F64Codec{})),
            makeOptionalDecodeField("name"_sv, codec::StringCodec{}));

        auto glTFAssetDecoder = makeRecordDecoder<GlTFAsset>(
            base::Constructor<GlTFAsset>{},
            makeOptionalDecodeField("copyright"_sv, codec::StringCodec{}),
            makeOptionalDecodeField("generator"_sv, codec::StringCodec{}),
            makeNormalDecodeField("version"_sv, codec::StringCodec{}),
            makeOptionalDecodeField("minVersion"_sv, codec::StringCodec{}));

        auto glTFBufferDecoder = makeRecordDecoder<GlTFBuffer>(
            base::Constructor<GlTFBuffer>{},
            makeOptionalDecodeField("uri"_sv, codec::StringCodec{}),
            makeNormalDecodeField("byteLength"_sv, codec::I32Codec{}),
            makeOptionalDecodeField("name"_sv, codec::StringCodec{}));

        auto glTFBufferViewDecoder = makeRecordDecoder<GlTFBufferView>(
            base::Constructor<GlTFBufferView>{},
            makeNormalDecodeField("buffer"_sv, codec::I32Codec{}),
            makeOptionalDecodeField("byteOffset"_sv, codec::I32Codec{}),
            makeNormalDecodeField("byteLength"_sv, codec::I32Codec{}),
            makeOptionalDecodeField("byteStride"_sv, codec::I32Codec{}),
            makeOptionalDecodeField("target"_sv, codec::I32Codec{}),
            makeOptionalDecodeField("name"_sv, codec::StringCodec{}));

        auto glTFMeshPrimitiveAttributesDecoder =
            makeRecordDecoder<GlTFMeshPrimitiveAttributes>(
                base::Constructor<GlTFMeshPrimitiveAttributes>{},
                makeOptionalDecodeField("POSITION"_sv, codec::I32Codec{}),
                makeOptionalDecodeField("NORMAL"_sv, codec::I32Codec{}),
                makeOptionalDecodeField("TEXCOORD_0"_sv, codec::I32Codec{}),
                makeOptionalDecodeField("COLOR_0"_sv, codec::I32Codec{}));

        auto glTFMeshPrimitiveDecoder = makeRecordDecoder<GlTFMeshPrimitive>(
            base::Constructor<GlTFMeshPrimitive>{},
            makeNormalDecodeField("attributes"_sv,
                                  glTFMeshPrimitiveAttributesDecoder),
            makeOptionalDecodeField("indices"_sv, codec::I32Codec{}),
            makeOptionalDecodeField("material"_sv, codec::I32Codec{}),
            makeOptionalDecodeField("mode"_sv, codec::I32Codec{}));

        auto glTFMeshDecoder = makeRecordDecoder<GlTFMesh>(
            base::Constructor<GlTFMesh>{},
            makeNormalDecodeField("primitives"_sv,
                                  makeVectorDecoder(glTFMeshPrimitiveDecoder)),
            makeOptionalDecodeField("weights"_sv,
                                    makeVectorDecoder(codec::F64Codec{})),
            makeOptionalDecodeField("name"_sv, codec::StringCodec{}));

        auto glTFNodeDecoder = makeRecordDecoder<GlTFNode>(
            base::Constructor<GlTFNode>{},
            makeOptionalDecodeField("camera"_sv, codec::I32Codec{}),
            makeOptionalDecodeField("children"_sv,
                                    makeVectorDecoder(codec::I32Codec{})),
            makeOptionalDecodeField("skin"_sv, codec::I32Codec{}),
            codec::makeOptionalDecodeField("matrix"_sv, Matrix4x4fDecoder{}),
            makeOptionalDecodeField("mesh"_sv, codec::I32Codec{}),
            codec::makeOptionalDecodeField("rotation"_sv, Vector4fDecoder{}),
            codec::makeOptionalDecodeField("scale"_sv, Vector3fDecoder{}),
            codec::makeOptionalDecodeField("translation"_sv, Vector3fDecoder{}),
            makeOptionalDecodeField("weights"_sv,
                                    makeVectorDecoder(codec::F64Codec{})),
            makeOptionalDecodeField("name"_sv, codec::StringCodec{}));

        auto glTFSceneDecoder = makeRecordDecoder<GlTFScene>(
            base::Constructor<GlTFScene>{},
            makeOptionalDecodeField("nodes"_sv,
                                    makeVectorDecoder(codec::I32Codec{})),
            makeOptionalDecodeField("name"_sv, codec::StringCodec{}));

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
            makeOptionalDecodeField("scene"_sv, codec::I32Codec{}),
            makeOptionalDecodeField("scenes"_sv,
                                    makeVectorDecoder(glTFSceneDecoder)));

    }  // namespace

    base::Either<GlTF, GlTFDecodeError> parseGlTF(const char* text, usize len) {
        auto json_result = json::JsonParser::parse(text, len);
        if (json_result.isRight())
            return base::Right<GlTFDecodeError>(json_result.right().message());

        codec::JsonDecodeBackend backend;
        auto result =
            glTFDecoder.decode(backend, base::move(json_result.left()));
        if (result.isLeft())
            return base::Left(base::move(result.left()));
        else
            return base::Right<GlTFDecodeError>(
                base::move(result.right().message()));
    }

}  // namespace spargel::gltf
