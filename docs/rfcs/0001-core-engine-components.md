# RFC 0001: Core Components Plan For The Rust Engine

- Status: Draft
- Date: 2026-02-23
- Scope: `crates/engine` and future engine subcrates

## Summary

This RFC defines the core components for Spargel Engine and the order to build them.  
The immediate goal is a playable vertical slice while keeping the architecture modular enough to grow into a full editor-driven engine.

## Goals

- Define the minimum core systems needed for a modern game runtime in Rust.
- Establish clear ownership boundaries between systems.
- Keep a path from the current simple `engine` crate to a multi-crate architecture.
- Prioritize deterministic runtime behavior, debuggability, and iteration speed.

## Non-Goals

- Locking the final API for all components.
- Building every subsystem to production depth in phase 1.
- Supporting every rendering backend or platform at launch (initial targets: Linux, macOS).

## Design Principles

- Data-oriented first: prefer contiguous, cache-friendly storage and explicit schedules.
- Explicit dependencies: systems communicate through typed resources/events, not globals.
- Thin unsafe boundary: contain backend/platform `unsafe` in small audited modules.
- Tooling-first runtime: diagnostics and editor integration are first-class requirements.
- Modular growth: start in `crates/engine`, split into subcrates when boundaries stabilize.
- Future‑ready: design with potential extensions in mind (networking, scripting) without over‑engineering.

## Terminology and API Contracts

- `Engine`: the runtime root object. Owns lifecycle, world state, schedules, and frame execution.
- `Plugin`: a compile-time composition unit that registers systems, resources, and events into an `Engine`.
- `Schedule`: a named execution phase (`Startup`, `FixedUpdate`, `Update`, `Render`) that orders system execution.
- `System`: executable logic unit run by a `Schedule`; reads/writes world data through declared access.
- `World`: ECS storage for entities, components, resources, and event channels.
- `Resource`: singleton world-scoped state (for example `Time`) shared across systems.
- `Event<T>`: typed message stream used for decoupled communication between systems.
- `Time`: canonical frame/fixed-step timing resource owned by the runtime.

Notes:
- API targets in this RFC are contract-level names; exact module paths and signatures may evolve.
- Rustdoc for implemented types should mirror these definitions to keep docs and code aligned.

## Core Components

### 1) Runtime, Engine Loop, and Plugin System (P0)

Responsibilities:
- Own engine startup/shutdown lifecycle.
- Drive fixed and variable update loops.
- Register systems/resources through plugins.
- Provide frame timing and task scheduling.

Primary API targets:
- `Engine`
- `Plugin`
- `Schedule` (`Startup`, `FixedUpdate`, `Update`, `Render`)
- `Time` resource

MVP exit criteria:
- Engine boots with a plugin list.
- Deterministic fixed-step simulation loop works.
- Headless mode can run schedules for tests.

### 2) ECS and Event System (P0)

Responsibilities:
- Manage entities/components/resources.
- Execute systems with borrow-safe query access.
- Route typed events/messages between systems.

Primary API targets:
- `World`
- `Entity`
- `Query`
- `Resource`
- `Event<T>`

MVP exit criteria:
- Spawn/despawn entities and query components efficiently.
- Event dispatch within a frame and optional buffered cross-frame events.
- Parallel system execution for independent queries/resources.

ECS implementation decision:
We will evaluate existing crates (hecs, specs, bevy_ecs) against our needs (parallelism, event system, reflection, determinism) and make a final choice by the end of the first sprint.
If no existing crate meets all requirements, we will build a minimal custom ECS with the necessary features.
This decision will be documented in a short follow‑up RFC.

### 3) Scene, Prefab, and Serialization (P0)

Responsibilities:
- Convert authored scene data to runtime entities/components.
- Support reusable prefab instances with overrides.
- Save/load scenes with stable IDs.

Primary API targets:
- `Scene`
- `Prefab`
- `SceneLoader`
- `Reflect`/schema metadata for serialization

MVP exit criteria:
- Load a scene file with transforms, mesh refs, and lights.
- Instantiate prefabs with per-instance overrides.
- Round-trip save/load without data loss for supported components.

### 4) Asset System and Content Pipeline (P0)

Responsibilities:
- Resolve asset handles and dependency graph.
- Async load and cache assets.
- Handle hot reload for editor/runtime iteration.

Primary API targets:
- `AssetServer`
- `Handle<T>`
- `AssetLoader`
- `AssetRegistry`

MVP exit criteria:
- Async load for textures, meshes, shaders, and scene files.
- Dependency tracking (scene -> prefab -> mesh/material).
- File watch hot reload for assets used by the running editor.

### 5) Rendering Framework (P0)

Responsibilities:
- Provide render-world extraction and render graph execution.
- Manage GPU resources and shader/material bindings.
- Support core passes: depth prepass (optional), opaque, transparent, UI.

Primary API targets:
- `RenderDevice`/`RenderQueue` abstraction over `wgpu`
- `RenderGraph`
- `Mesh`, `Material`, `Camera`, `Light`
- `RendererPlugin`

MVP exit criteria:
- Draw static meshes with PBR-lite materials and directional light.
- Camera controls + viewport resize correctness.
- Editor viewport integration (replacing one-off triangle path).

### 6) Platform and Input Layer (P0)

Responsibilities:
- Abstract window/event backend and input state.
- Normalize keyboard/mouse/gamepad input mapping.
- Provide per-frame action state and raw device events.

Primary API targets:
- `Input<TAction>`
- `ActionMap`
- `WindowState`

MVP exit criteria:
- Keyboard + mouse actions bound through config.
- Relative mouse look and UI focus-aware input routing.
- Window lifecycle events routed into the app schedule.

### 7) Physics Integration (P1)

Responsibilities:
- Collision, rigid body simulation, and character controller hooks.
- Sync transforms between ECS world and physics world.

Primary API targets:
- `PhysicsWorld`
- `RigidBody`
- `Collider`
- `PhysicsStepPlugin`

MVP exit criteria:
- Basic dynamic/static rigid bodies and collision events.
- Fixed timestep simulation with interpolation support.

### 8) Audio System (P1)

Responsibilities:
- Playback for one-shot and spatial audio.
- Streaming for music/long clips.
- Mixer buses and volume controls.

Primary API targets:
- `AudioSource`
- `AudioEmitter`
- `AudioListener`
- `AudioMixer`

MVP exit criteria:
- Play 2D/3D sounds with attenuation and pause/resume.
- Mixer groups (`master`, `music`, `sfx`) exposed to editor UI.

### 9) UI and Editor Integration (P1)

Responsibilities:
- Runtime debug UI and editor widgets.
- Inspector integration via reflection metadata.
- Gizmos and scene hierarchy views.

Primary API targets:
- `UiPlugin`
- `InspectorRegistry`
- `GizmoContext`

MVP exit criteria:
- Inspect/edit core components (transform, camera, light, mesh renderer).
- Scene hierarchy + selection + transform gizmo operations.

### 10) Diagnostics, Testing, and Tooling (P0)

Responsibilities:
- Logging, tracing, metrics, and frame capture hooks.
- Deterministic simulation tests and golden rendering checks.
- CI checks for perf regressions and API health.

Primary API targets:
- `DiagnosticsPlugin`
- `FrameStats`
- `TraceCapture`

MVP exit criteria:
- Per-frame timings for update/render stages.
- Smoke tests that run headless in CI.
- Debug overlays for FPS, frame time, draw calls.

## Suggested Module Layout (Near-Term)

Start inside `crates/engine/src` with:

- `app/` (runtime + plugin + schedule; public facade exports `Engine`/`Plugin`)
- `ecs/`
- `scene/`
- `asset/`
- `render/`
- `platform/`
- `input/`
- `diagnostics/`

When APIs stabilize, split into subcrates:

- `engine_app`, `engine_ecs`, `engine_scene`, `engine_asset`, `engine_render`, `engine_input`
- Keep `engine` as a facade/re-export crate for ergonomics.

## Delivery Plan

### Phase 1: Vertical Slice (P0 only)

- Runtime + ECS + scene loading + assets + rendering + input + diagnostics.
- Deliverable: controllable camera in editor viewport rendering a scene loaded from assets.

### Phase 2: Gameplay Foundations

- Physics + audio + basic prefab workflow improvements.
- Deliverable: small playable sample with collisions and sound.

### Phase 3: Editor Maturity

- Inspector expansion, gizmos, richer debugging and profiling.
- Deliverable: practical authoring workflow for iterative scene editing.

## Risks and Mitigations

- ECS complexity risk: start with a minimal scheduler and add parallelism only after correctness tests.
- Render architecture churn: define extraction boundary early to decouple gameplay from render world.
- Asset hot-reload instability: centralize dependency tracking and explicit reload events.
- Editor/runtime coupling: keep editor features as plugins using stable runtime APIs.

## Open Questions

- Build-vs-buy for ECS: custom ECS or integration with an existing crate.
- Reflection strategy: custom derive metadata vs existing ecosystem crates.
- Physics/audio backends: choose libraries and abstraction depth.
- Scriptability: if needed, embed scripting now or defer until gameplay API is stable.
