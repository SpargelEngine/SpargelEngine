#include <spargel/base/algorithm.h>
#include <spargel/base/assert.h>
#include <spargel/base/attribute.h>
#include <spargel/base/logging.h>
#include <spargel/base/optional.h>
#include <spargel/base/tag_invoke.h>
#include <spargel/math/matrix.h>
#include <spargel/math/point.h>
#include <spargel/math/vector.h>
#include <spargel/resource/directory.h>
#include <spargel/ui/platform.h>
#include <spargel/ui/ui_mac.h>
#include <spargel/ui/window.h>

//
#include <spargel/render/shader/metal_shader.generated.h>

// libc
#include <stdio.h>

//
#import <Metal/Metal.h>
#import <simd/simd.h>

using namespace spargel;

using math::Matrix4x4f;
using math::Point3f;
using math::Vector3f;

class Cursor final {
public:
    explicit Cursor(base::span<u8> bytes) : _begin{bytes.begin()}, _end{bytes.end()}, _cur{_begin} {
        printf("len = %zu\n", bytes.count());
    }

    base::Optional<u32> readU32() {
        eatWhitespace();
        u32 result = 0;
        u8 c = peek();
        if (c < '0' || c > '9') {
            return {};
        }
        while (!isEnd()) {
            c = peek();
            if (c < '0' || c > '9') {
                break;
            }
            result = result * 10 + (c - '0');
            advance();
        }
        return base::makeOptional<u32>(result);
    }

    base::Optional<float> readFloat() {
        eatWhitespace();
        float result = 0;
        float d = 1;
        u8 c = peek();
        if ((c < '0' || c > '9') && (c != '-')) {
            return {};
        }
        bool negative = false;
        if (c == '-') {
            negative = true;
            advance();
        }
        bool is_decimal = false;
        while (!isEnd()) {
            c = peek();
            if (c == '.') {
                is_decimal = true;
                advance();
                continue;
            }
            if (c < '0' || c > '9') {
                break;
            }
            u8 digit = c - '0';
            if (is_decimal) {
                d *= 10;
                result += digit / d;
            } else {
                result = result * 10 + digit;
            }
            advance();
        }
        if (negative) {
            result = -result;
        }
        return base::makeOptional<float>(result);
    }

private:
    bool isEnd() const { return _cur >= _end; }

    u8 peek() const {
        if (isEnd()) {
            return 0;
        }
        return *_cur;
    }

    void advance() {
        if (isEnd()) {
            return;
        }
        _cur++;
    }

    void eatWhitespace() {
        while (!isEnd()) {
            u8 c = peek();
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                advance();
                continue;
            } else if (c == '#') {
                eatUntil('\n');
            } else {
                break;
            }
        }
    }

    void eatUntil(u8 ch) {
        while (!isEnd()) {
            u8 c = peek();
            if (c == ch) {
                break;
            }
            advance();
        }
    }

    u8 const* _begin;
    u8 const* _end;
    u8 const* _cur;
};

float min(float x, float y) { return x < y ? x : y; }
float max(float x, float y) { return x > y ? x : y; }

struct BoundingBox {
    float xmin;
    float ymin;
    float zmin;
    float xmax;
    float ymax;
    float zmax;

    BoundingBox() = default;
    BoundingBox(float x, float y, float z) : xmin{x}, ymin{y}, zmin{z}, xmax{x}, ymax{y}, zmax{z} {}

    void merge(float x, float y, float z) {
        xmin = min(xmin, x);
        ymin = min(ymin, y);
        zmin = min(zmin, z);
        xmax = max(xmax, x);
        ymax = max(ymax, y);
        zmax = max(zmax, z);
    }
};

class SimpleMesh final {
public:
    static base::Optional<SimpleMesh> create(base::span<u8> bytes) {
        Cursor cursor(bytes);

        auto face_count = cursor.readU32();
        auto vert_count = cursor.readU32();

        if (!face_count.hasValue() || !vert_count.hasValue()) {
            return {};
        }

        base::vector<u32> indices;
        base::vector<Point3f> vertices;

        u32 indices_count = face_count.value() * 3;
        indices.reserve(indices_count);
        indices.set_count(indices_count);

        for (u32 i = 0; i < indices_count; i++) {
            auto value = cursor.readU32();
            if (!value.hasValue()) {
                spargel_log_error("bad index %d", i);
                return {};
            }
            indices[i] = value.value();
        }

        vertices.reserve(vert_count.value());
        vertices.set_count(vert_count.value());

        for (u32 i = 0; i < vert_count.value(); i++) {
            auto x = cursor.readFloat();
            auto y = cursor.readFloat();
            auto z = cursor.readFloat();
            if (!x.hasValue() || !y.hasValue() || !z.hasValue()) {
                spargel_log_error("bad point %d", i);
                return {};
            }
            // scale the bunny mesh
            vertices[i] = {x.value() * 1000, y.value() * 1000, z.value() * 1000};
        }

        return base::makeOptional<SimpleMesh>(face_count.value(), vert_count.value(),
                                              base::move(indices), base::move(vertices));
    }

    SimpleMesh() = default;

    SimpleMesh(u32 face_count, u32 vert_count, base::vector<u32> indices,
               base::vector<Point3f> vertices)
        : _face_count{face_count},
          _vert_count{vert_count},
          _indices(base::move(indices)),
          _vertices(base::move(vertices)) {
        _bbox = BoundingBox(_vertices[0].x, _vertices[0].y, _vertices[0].z);
        for (usize i = 0; i < _vertices.count(); i++) {
            _bbox.merge(_vertices[i].x, _vertices[i].y, _vertices[i].z);
        }
        computeNormals();
    }

    SimpleMesh(SimpleMesh const& other) = default;
    SimpleMesh& operator=(SimpleMesh const& other) {
        SimpleMesh tmp(other);
        base::swap(*this, tmp);
        return *this;
    }

    SimpleMesh(SimpleMesh&& other) { base::swap(*this, other); }
    SimpleMesh& operator=(SimpleMesh&& other) {
        SimpleMesh tmp(base::move(other));
        base::swap(*this, tmp);
        return *this;
    }

    u32 getFaceCount() const { return _face_count; }
    u32 getVertexCount() const { return _vert_count; }

    base::span<u32> getIndices() const { return _indices.toSpan(); }
    base::span<Point3f> getVertices() const { return _vertices.toSpan(); }
    base::span<Vector3f> getNormals() const { return _normals.toSpan(); }

    BoundingBox const& getBoundingBox() const { return _bbox; }

    friend void tag_invoke(base::tag<base::swap>, SimpleMesh& lhs, SimpleMesh& rhs) {
        base::swap(lhs._face_count, rhs._face_count);
        base::swap(lhs._vert_count, rhs._vert_count);
        base::swap(lhs._indices, rhs._indices);
        base::swap(lhs._vertices, rhs._vertices);
        base::swap(lhs._normals, rhs._normals);
        base::swap(lhs._bbox, rhs._bbox);
    }

private:
    void computeNormals() {
        _normals.reserve(_vertices.count());
        _normals.set_count(_vertices.count());

        for (usize i = 0; i < _indices.count(); i += 3) {
            Point3f const& x = _vertices[_indices[i]];
            Point3f const& y = _vertices[_indices[i + 1]];
            Point3f const& z = _vertices[_indices[i + 2]];

            _normals[_indices[i]] += (z - x).cross(y - x);
            _normals[_indices[i + 1]] += (x - y).cross(z - z);
            _normals[_indices[i + 2]] += (y - z).cross(x - z);
        }

        // normalize
        for (usize i = 0; i < _normals.count(); i++) {
            _normals[i] = _normals[i].normalize();
        }
    }

    u32 _face_count = 0;
    u32 _vert_count = 0;
    base::vector<u32> _indices;
    base::vector<Point3f> _vertices;
    base::vector<Vector3f> _normals;
    BoundingBox _bbox;
};

struct ControlData {
    Matrix4x4f model_to_world;
    Matrix4x4f world_to_camera;
    Matrix4x4f camera_to_clip;
};

static_assert(sizeof(ControlData) == sizeof(float) * 16 * 3);

/// A simple perspective or parallel camera.
class Camera {
public:
    Camera() = default;

    /// Move the camera by the vector.
    void move(Vector3f v) {
        _position = _position + v;
        _state_dirty = true;
    }

    void setPosition(Point3f p) {
        _position = p;
        _state_dirty = true;
    }
    void setDirection(Vector3f v) {
        _direction = v;
        _state_dirty = true;
    }
    void setUp(Vector3f v) {
        _up = v;
        _state_dirty = true;
    }

    void setViewAngle(float v) {
        _view_angle = v;
        _state_dirty = true;
    }
    void setAspectRatio(float r) {
        _aspect_ratio = r;
        _state_dirty = true;
    }
    void setNearPlane(float d) {
        _near_plane = d;
        _state_dirty = true;
    }
    void setFarPlane(float d) {
        _far_plane = d;
        _state_dirty = true;
    }

    void update() {
        if (!_state_dirty) {
            return;
        }
        computeWorldToCamera();
        computeCameraToClip();
        _state_dirty = false;
    }

    Matrix4x4f const& getWorldToCamera() const { return _world_to_camera; }
    Matrix4x4f const& getCameraToClip() const { return _camera_to_clip; }

private:
    void computeWorldToCamera() {
        Vector3f z = _direction.normalize();
        Vector3f y = _up.normalize();
        // The coordinate system is left-handed.
        Vector3f x = _up.cross(_direction).normalize();
        Vector3f p = _position.asVector();
        Vector3f t(p.dot(x), p.dot(y), p.dot(z));
        // clang-format off
        _world_to_camera = Matrix4x4f(
            x.x, y.x, z.x, 0,
            x.y, y.y, z.y, 0,
            x.z, y.z, z.z, 0,
            -t.x, -t.y, -t.z, 1
        );
        // clang-format on
    }
    void computeCameraToClip() {
        // y = tan(a / 2), x = r * y
        float ys = 1.0 / math::tan(_view_angle * 0.5);
        float xs = ys / _aspect_ratio;
        float zs = _far_plane / (_far_plane - _near_plane);
        float t = -zs * _near_plane;
        // clang-format off
        _camera_to_clip = Matrix4x4f(
            xs, 0, 0, 0,
            0, ys, 0, 0,
            0, 0, zs, 1,
            0, 0, t, 0
        );
        // clang-format on
    }

    // The position of the camera in the world coordinate.
    Point3f _position;
    // The facing direction of the camera.
    Vector3f _direction;
    // The up direction of the camera.
    Vector3f _up;

    // The angle of view in the vertical direction, in radians.
    float _view_angle;
    // The ratio of width over height;
    float _aspect_ratio;
    // The distance to the near plane;
    float _near_plane;
    // The distance to the far plane;
    float _far_plane;

    Matrix4x4f _world_to_camera;
    Matrix4x4f _camera_to_clip;

    bool _state_dirty = true;
};

class Delegate final : public ui::WindowDelegate {
public:
    Delegate(ui::WindowAppKit* window) : _window{window} { _Setup(); }

    void onKeyboard(ui::KeyboardEvent& e) override {
        float d = 10;
        float dx = 0;
        float dy = 0;
        float dz = 0;
        if (e.key == ui::PhysicalKey::key_w) {
            dy += d;
        } else if (e.key == ui::PhysicalKey::key_s) {
            dy -= d;
        } else if (e.key == ui::PhysicalKey::key_a) {
            dx += d;
        } else if (e.key == ui::PhysicalKey::key_d) {
            dx -= d;
        } else if (e.key == ui::PhysicalKey::key_j) {
            dz += d;
        } else if (e.key == ui::PhysicalKey::key_k) {
            dz -= d;
        }
        _camera.move(Vector3f(dx, dy, dz));

        _window->requestRedraw();
    }

    void onRender() override {
        auto cmdbuf = [_queue commandBuffer];

        auto drawable = [_layer nextDrawable];

        auto desc = [MTLRenderPassDescriptor renderPassDescriptor];
        desc.colorAttachments[0].texture = drawable.texture;
        desc.colorAttachments[0].loadAction = MTLLoadActionClear;
        desc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 1.0, 1.0, 1.0);

        auto encoder = [cmdbuf renderCommandEncoderWithDescriptor:desc];

        [encoder setRenderPipelineState:_mesh_pipeline];

        // for bunny mesh
        [encoder setFrontFacingWinding:MTLWindingCounterClockwise];
        [encoder setCullMode:MTLCullModeBack];

        [encoder setVertexBuffer:_vertices offset:0 atIndex:0];
        [encoder setVertexBuffer:_normals offset:0 atIndex:1];

        float ratio = _layer.drawableSize.width / _layer.drawableSize.height;
        _camera.setAspectRatio(ratio);

        _camera.update();

        _cd.world_to_camera = _camera.getWorldToCamera();
        _cd.camera_to_clip = _camera.getCameraToClip();

        [encoder setVertexBytes:&_cd length:sizeof(ControlData) atIndex:2];

        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                            indexCount:_mesh.getIndices().count()
                             indexType:MTLIndexTypeUInt32
                           indexBuffer:_indices
                     indexBufferOffset:0];

        [encoder endEncoding];

        [cmdbuf presentDrawable:drawable];
        [cmdbuf commit];
    }

private:
    void _Setup() {
        NSError* error;

        _resource = resource::makeRelativeManager(base::string("resources"));

        {
            auto blob = _resource->open(resource::ResourceId("bunny.smesh"));
            if (!blob.hasValue()) {
                spargel_log_error("cannot load mesh");
                spargel_panic_here();
            }

            auto s = SimpleMesh::create(blob.value()->getSpan());
            if (!s.hasValue()) {
                spargel_log_error("wrong format");
                spargel_panic_here();
            }
            _mesh = base::move(s).value();

            auto& bbox = _mesh.getBoundingBox();
            spargel_log_info("bounding box: (%.3f, %.3f, %.3f) - (%.3f, %.3f, %.3f)", bbox.xmin,
                             bbox.ymin, bbox.zmin, bbox.xmax, bbox.ymax, bbox.zmax);
        }

        _device = MTLCreateSystemDefaultDevice();
        _queue = [_device newCommandQueue];

        {
            _layer = (CAMetalLayer*)_window->getHandle().apple.layer;
            _layer.device = _device;
        }

        {
            // auto blob = _resource->open(resource::ResourceId("shader.metallib"));
            // if (!blob.hasValue()) {
            //     spargel_log_error("cannot load shader");
            //     spargel_panic_here();
            // }
            // auto blob_span = blob.value()->getSpan();
            auto library_data =
                // dispatch_data_create(blob_span.data(), blob_span.count(), dispatch_get_main_queue(),
                dispatch_data_create(spargel_render_metal_shader, sizeof(spargel_render_metal_shader), dispatch_get_main_queue(),
                                     DISPATCH_DATA_DESTRUCTOR_DEFAULT);

            _library = [_device newLibraryWithData:library_data error:&error];

            _vert_func = [_library newFunctionWithName:@"vertex_shader"];
            _frag_func = [_library newFunctionWithName:@"fragment_shader"];
        }

        {
            auto vert_desc = [MTLVertexDescriptor vertexDescriptor];
            // Buffer 0 is vertices.
            vert_desc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
            vert_desc.layouts[0].stride = sizeof(Point3f);
            // Buffer 1 is normals.
            vert_desc.layouts[1].stepFunction = MTLVertexStepFunctionPerVertex;
            vert_desc.layouts[1].stride = sizeof(Vector3f);
            // Attribute 0 is vertex position.
            vert_desc.attributes[0].format = MTLVertexFormatFloat3;
            vert_desc.attributes[0].offset = 0;
            vert_desc.attributes[0].bufferIndex = 0;
            vert_desc.attributes[1].format = MTLVertexFormatFloat3;
            vert_desc.attributes[1].offset = 0;
            vert_desc.attributes[1].bufferIndex = 1;

            auto desc = [[MTLRenderPipelineDescriptor alloc] init];
            desc.vertexFunction = _vert_func;
            desc.fragmentFunction = _frag_func;
            desc.vertexDescriptor = vert_desc;

            desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
            // desc.colorAttachments[0].blendingEnabled = false;

            desc.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;

            _mesh_pipeline = [_device newRenderPipelineStateWithDescriptor:desc error:&error];
        }

        {
            _vertices = [_device newBufferWithBytes:_mesh.getVertices().asBytes().data()
                                             length:_mesh.getVertices().asBytes().count()
                                            options:MTLResourceStorageModeShared];
            _indices = [_device newBufferWithBytes:_mesh.getIndices().asBytes().data()
                                            length:_mesh.getIndices().asBytes().count()
                                           options:MTLResourceStorageModeShared];
            _normals = [_device newBufferWithBytes:_mesh.getNormals().asBytes().data()
                                            length:_mesh.getNormals().asBytes().count()
                                           options:MTLResourceStorageModeShared];
        }

        _camera.setDirection(Vector3f(0, 0, 1));
        _camera.setUp(Vector3f(0, 1, 0));
        _camera.setViewAngle(3.14159 / 3);  // 60 degrees
        _camera.setNearPlane(1);
        _camera.setFarPlane(1000);

        _window->setAnimating(false);
    }

    ui::WindowAppKit* _window;
    base::unique_ptr<resource::ResourceManagerDirectory> _resource;

    SimpleMesh _mesh;

    id<MTLDevice> _device;
    id<MTLCommandQueue> _queue;
    id<MTLRenderPipelineState> _mesh_pipeline;
    id<MTLLibrary> _library;
    id<MTLFunction> _vert_func;
    id<MTLFunction> _frag_func;

    id<MTLBuffer> _vertices;
    id<MTLBuffer> _indices;
    id<MTLBuffer> _normals;

    CAMetalLayer* _layer;

    Camera _camera;

    // clang-format off
    ControlData _cd = {
        .model_to_world =
            Matrix4x4f(
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 500, 1
            ),
    };
    // clang-format on
};

int main() {
    auto platform = ui::makePlatform();

    auto window = platform->makeWindow(500, 500);
    window->setTitle("Spargel Demo - Render");

    Delegate d(static_cast<ui::WindowAppKit*>(window.get()));
    window->setDelegate(&d);

    platform->startLoop();
    return 0;
}
