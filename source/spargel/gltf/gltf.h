/*
 * A glTF 2.0 parser implementation
 *
 * This is base on the specification document *glTF 2.0 Specification*
 * (Version 2.0.1)
 *
 * Some of the comments are taken and adapted from this document.
 */

#pragma once

#include "spargel/base/optional.h"
#include "spargel/base/string.h"
#include "spargel/base/vector.h"
#include "spargel/codec/codec.h"
#include "spargel/math/matrix.h"
#include "spargel/math/vector.h"

namespace spargel::gltf {

    using GlTFBoolean = bool;
    using GlTFInteger = i32;
    using GlTFNumber = f64;
    using GlTFString = base::String;

    /*
     * A typed view into a buffer view that contains raw binary data.
     */
    struct GlTFAccessor {
        // The index of the buffer view. When undefined, the accessor MUST be
        // initialized with zeros; sparse property or extensions MAY override
        // zeros with actual values. (>=0)
        base::Optional<GlTFInteger> bufferView;

        // The offset relative to the start of the buffer view in bytes. This
        // MUST be a multiple of the size of the component datatype. This
        // property MUST NOT be defined when bufferView is undefined. (>=0)
        // Default: 0.
        base::Optional<GlTFInteger> byteOffset;

        // The datatype of the accessor’s components. UNSIGNED_INT type MUST NOT
        // be used for any accessor that is not referenced by
        // mesh.primitive.indices. Allowed: 5120=BYTE, 5121=UNSIGNED_BYTE,
        // 5122=SHORT, 5123=UNSIGNED_SHORT,
        //   5125=UNSIGNED_INT, 5126=FLOAT
        GlTFInteger componentType;

        // Specifies whether integer data values are normalized (true) to [0, 1]
        // (for unsigned types) or to [-1, 1] (for signed types) when they are
        // accessed. This property MUST NOT be set to true for accessors with
        // FLOAT or UNSIGNED_INT component type. Default: false.
        base::Optional<GlTFBoolean> normalized;

        // The number of elements referenced by this accessor, not to be
        // confused with the number of bytes or number of components. (>=1)
        GlTFInteger count;

        // Specifies if the accessor’s elements are scalars, vectors, or
        // matrices. Allowed: "SCALAR", "VEC2", "VEC3", "VEC4", "MAT2", "MAT3",
        // "MAT4".
        GlTFString type;

        // Maximum value of each component in this accessor. Array elements MUST
        // be treated as having the same data type as accessor’s componentType.
        // Both min and max arrays have the same length. The length is
        // determined by the value of the type property; it can be 1, 2, 3, 4,
        // 9, or 16.
        base::Optional<base::Vector<GlTFNumber>> max;

        // Minimum value of each component in this accessor. Array elements MUST
        // be treated as having the same data type as accessor’s componentType.
        // Both min and max arrays have the same length. The length is
        // determined by the value of the type property; it can be 1, 2, 3, 4,
        // 9, or 16.
        base::Optional<base::Vector<GlTFNumber>> min;

        // TODO: sparse

        // The user-defined name of this object. This is not necessarily unique,
        // e.g., an accessor and a buffer could have the same name, or two
        // accessors could even have the same name.
        base::Optional<GlTFString> name;
    };

    /* TODO
     * Sparse storage of accessor values that deviate from their initialization
     * value.
     */

    /*
     * Metadata about the glTF asset.
     */
    struct GlTFAsset {
        // A copyright message suitable for display to credit the content
        // creator.
        base::Optional<GlTFString> copyright;

        // Tool that generated this glTF model. Useful for debugging.
        base::Optional<GlTFString> generator;

        // The glTF version in the form of <major>.<minor> that this asset
        // targets. Pattern: ^[0-9]+\.[0-9]+$
        GlTFString version;

        // The minimum glTF version in the form of <major>.<minor> that this
        // asset targets. This property MUST NOT be greater than the asset
        // version. Pattern: ^[0-9]+\.[0-9]+$
        base::Optional<GlTFString> minVersion;
    };

    /*
     * A buffer points to binary geometry, animation, or skins.
     */
    struct GlTFBuffer {
        // The URI (or IRI) of the buffer. Relative paths are relative to the
        // current glTF asset. Instead of referencing an external file, this
        // field MAY contain a data:-URI. Format: iri-reference.
        base::Optional<GlTFString> uri;

        // The length of the buffer in bytes. (>= 1)
        GlTFInteger byteLength;

        // The user-defined name of this object. This is not necessarily unique,
        // e.g., an accessor and a buffer could have the same name, or two
        // accessors could even have the same name.
        base::Optional<GlTFString> name;
    };

    /*
     * A view into a buffer generally representing a subset of the buffer.
     */
    struct GlTFBufferView {
        // The index of the buffer. (>=0)
        GlTFInteger buffer;

        // The offset into the buffer in bytes. (>= 0)
        // Default: 0.
        base::Optional<GlTFInteger> byteOffset;

        // The length of the bufferView in bytes. (>= 1)
        GlTFInteger byteLength;

        // The stride, in bytes, between vertex attributes. When this is not
        // defined, data is tightly packed. When two or more accessors use the
        // same buffer view, this field MUST be defined. (>= 4, <= 252)
        base::Optional<GlTFInteger> byteStride;

        // The hint representing the intended GPU buffer type to use with this
        // buffer view. Allowed: 34962=ARRAY_BUFFER, 34963=ELEMENT_ARRAY_BUFFER.
        base::Optional<GlTFInteger> target;

        // The user-defined name of this object. This is not necessarily unique,
        // e.g., an accessor and a buffer could have the same name, or two
        // accessors could even have the same name.
        base::Optional<GlTFString> name;
    };

    /*
     * A set of primitives to be rendered.
     */
    struct GlTFMeshPrimitive;
    struct GlTFMesh {
        // An array of primitives, each defining geometry to be rendered.
        base::Vector<GlTFMeshPrimitive> primitives;

        // Array of weights to be applied to the morph targets.
        // The number of array elements MUST match the number of morph targets.
        base::Optional<base::Vector<GlTFNumber>> weights;

        // The user-defined name of this object. This is not necessarily unique,
        // e.g., an accessor and a buffer could have the same name, or two
        // accessors could even have the same name.
        base::Optional<GlTFString> name;
    };

    struct GlTFMeshPrimitiveAttributes {
        // Unitless XYZ vertex positions
        // VEC3; float
        base::Optional<GlTFInteger> position;

        // Normalized XYZ vertex normals
        // VEC3; float
        base::Optional<GlTFInteger> normal;

        // ST texture coordinates
        // VEC2; float, unsigned byte normalized, unsigned short normalized
        base::Optional<GlTFInteger> texcoord_0;

        // RGB or RGBA vertex color linear multiplier
        // VEC3, VEC4; float, unsigned byte normalized, unsigned short
        // normalized
        base::Optional<GlTFInteger> color_0;
    };

    /*
     * Geometry to be rendered with the given material.
     */
    struct GlTFMeshPrimitive {
        // A plain JSON object, where each key corresponds to a mesh attribute
        // semantic and each value is the index of the accessor containing
        // attribute's data. (HashMap<GlTFString, GlTFInteger> attributes;)
        GlTFMeshPrimitiveAttributes attributes;

        // The index of the accessor that contains the vertex indices. (>=0)
        base::Optional<GlTFInteger> indices;

        // The index of the material to apply to this primitive when rendering.
        // (>=0)
        base::Optional<GlTFInteger> material;

        // The topology type of primitives to render.
        // Allowed: 0=POINTS, 1=LINES, 2=LINE_LOOP, 3=LINE_STRIP, 4=TRIANGLES,
        // 5=TRIANGLE_STRIP, 6=TRIANGLE_FAN. Default: 4.
        base::Optional<GlTFInteger> mode;
    };

    /*
     * A node in the node hierarchy.
     */
    struct GlTFNode {
        // The index of the camera referenced by this node. (>=0)
        base::Optional<GlTFInteger> camera;

        // The indices of this node’s children. (unique, >= 0)
        base::Optional<base::Vector<GlTFInteger>> children;

        // The index of the skin referenced by this node. When a skin is
        // referenced by a node within a scene, all joints used by the skin MUST
        // belong to the same scene. When defined, mesh MUST also be defined.
        // (>=0)
        base::Optional<GlTFInteger> skin;

        // A floating-point 4x4 transformation matrix stored in column-major
        // order. Default: [1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1].
        base::Optional<math::Matrix4x4f> matrix;

        // The index of the mesh in this node. (>= 0)
        base::Optional<GlTFInteger> mesh;

        // The node’s unit quaternion rotation in the order (x, y, z, w), where
        // w is the scalar. Default:[0,0,0,1].
        base::Optional<math::Vector4f> rotation;

        // The node’s non-uniform scale, given as the scaling factors along the
        // x, y, and z axes. Default: [1,1,1].
        base::Optional<math::Vector3f> scale;

        // The node’s translation along the x, y, and z axes.
        // Default: [0,0,0].
        base::Optional<math::Vector3f> translation;

        // The weights of the instantiated morph target. The number of array
        // elements MUST match the number of morph targets of the referenced
        // mesh. When defined, mesh MUST also be defined.
        base::Optional<base::Vector<GlTFNumber>> weights;

        // The user-defined name of this object. This is not necessarily unique,
        // e.g., an accessor and a buffer could have the same name, or two
        // accessors could even have the same name.
        base::Optional<GlTFString> name;
    };

    /*
     * The root nodes of a scene.
     */
    struct GlTFScene {
        // The indices of each root node. (unique, >= 0)
        base::Optional<base::Vector<GlTFInteger>> nodes;

        // The user-defined name of this object. This is not necessarily unique,
        // e.g., an accessor and a buffer could have the same name, or two
        // accessors could even have the same name.
        base::Optional<GlTFString> name;
    };

    /*
     * The root object for a glTF asset.
     */
    struct GlTF {
        // An array of accessors. An accessor is a typed view into a bufferView.
        base::Optional<base::Vector<GlTFAccessor>> accessors;

        // Metadata about the glTF asset.
        GlTFAsset asset;

        // An array of buffers. A buffer points to binary geometry, animation,
        // or skins.
        base::Optional<base::Vector<GlTFBuffer>> buffers;

        // An array of bufferViews. A bufferView is a view into a buffer
        // generally representing a subset of the buffer.
        base::Optional<base::Vector<GlTFBufferView>> bufferViews;

        // An array of meshes. A mesh is a set of primitives to be rendered.
        base::Optional<base::Vector<GlTFMesh>> meshes;

        // An array of nodes.
        base::Optional<base::Vector<GlTFNode>> nodes;

        // The index of the default scene. This property MUST NOT be defined,
        // when scenes is undefined. (>= 0)
        base::Optional<GlTFInteger> scene;

        // An array of scenes.
        base::Optional<base::Vector<GlTFScene>> scenes;
    };

    using GlTFDecodeError = codec::CodecError;

    base::Either<GlTF, GlTFDecodeError> parseGlTF(const char* text, usize len);

}  // namespace spargel::gltf
