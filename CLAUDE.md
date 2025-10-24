# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is the **Godot Engine** customized for the spx project. It is a fork of Godot located at `pkg/gdspx/godot/` within the spx repository. The key customization is the **modules/spx/** directory, which provides all spx-specific functionality.

**IMPORTANT**: When working in this directory, focus on the `modules/spx/` directory - this is the core integration layer for spx, not the standard Godot engine code.

## Building Commands

```bash
# Build Godot with spx module enabled
scons platform=macos target=template_release spx=yes

# Build for different platforms
scons platform=linux target=template_release spx=yes
scons platform=windows target=template_release spx=yes
scons platform=web target=template_release spx=yes
scons platform=android target=template_release spx=yes
scons platform=ios target=template_release spx=yes

# Build editor version (for debugging)
scons platform=macos target=editor spx=yes

# Common optimization flags used in spx builds
scons platform=<platform> target=<target> \
    spx=yes \
    optimize=size \
    disable_3d=true \
    disable_3d_physics=true \
    vulkan=false \
    module_gdscript_enabled=true \
    module_godot_physics_2d_enabled=true
```

## modules/spx/ - Core Integration Layer

**CRITICAL**: This is the most important directory for spx development. All spx-specific Godot functionality is here.

### Module Overview

The `modules/spx/` directory contains C++ code that extends Godot to support spx's game engine requirements. It is enabled via the `spx=yes` SCons build flag (see `config.py`).

**Key Entry Points**:
- `register_types.cpp`: Registers spx module classes (SpxDrawTiles, SpxPathFinder, PathDebugDrawer)
- `spx.h/cpp`: Main static class providing lifecycle hooks and pause functionality
- `spx_engine.h/cpp`: Singleton managing all subsystem managers
- `gdextension_spx_ext.h/cpp`: FFI interface for Go-Godot communication

### Architecture: Manager-Based System

All functionality is organized into specialized managers, each inheriting from `SpxBaseMgr`:

**Core Managers** (accessed via `SpxEngine::get_singleton()->get_*()`):
- **SpxSpriteMgr** (`spx_sprite_mgr.h/cpp`): Sprite lifecycle, animation, physics modes
- **SpxSceneMgr** (`spx_scene_mgr.h/cpp`): Scene graph, pure sprites (render-only), static sprites
- **SpxPhysicMgr** (`spx_physic_mgr.h/cpp`): Physics queries, collision/trigger events
- **SpxAudioMgr** (`spx_audio_mgr.h/cpp`): Sound playback, audio bus management
- **SpxInputMgr** (`spx_input_mgr.h/cpp`): Keyboard/mouse/gamepad input
- **SpxCameraMgr** (`spx_camera_mgr.h/cpp`): Camera control and following
- **SpxUiMgr** (`spx_ui_mgr.h/cpp`): UI widgets and events
- **SpxResMgr** (`spx_res_mgr.h/cpp`): Texture, sound, font resource loading
- **SpxNavigationMgr** (`spx_navigation_mgr.h/cpp`): Navigation meshes
- **SpxPenMgr** (`spx_pen_mgr.h/cpp`): Drawing API (Scratch pen feature)
- **SpxTilemapMgr** (`spx_tilemap_mgr.h/cpp`): Tilemap support
- **SpxPlatformMgr** (`spx_platform_mgr.h/cpp`): Platform-specific utilities
- **SpxExtMgr** (`spx_ext_mgr.h/cpp`): Extension management
- **SpxDebugMgr** (`spx_debug_mgr.h/cpp`): Debug utilities

**Manager Lifecycle**:
All managers implement:
- `on_awake()`: Initialization when engine starts
- `on_update(float delta)`: Called every frame
- `on_fixed_update(float delta)`: Called at fixed physics timestep
- `on_destroy()`: Cleanup when engine shuts down

### FFI and Callbacks

**Go ↔ C++ Communication** via `gdextension_spx_ext.h`:

**Type Mappings**:
```cpp
typedef GDExtensionConstStringPtr GdString;
typedef GDExtensionInt GdInt;
typedef GDExtensionInt GdObj;  // Object ID
typedef GDExtensionBool GdBool;
typedef real_t GdFloat;
typedef Vector2 GdVec2;
typedef Vector3 GdVec3;
typedef Vector4 GdVec4;
typedef Color GdColor;
typedef Rect2 GdRect2;
```

**Callback System** (`SpxCallbackInfo` struct):
- **Engine lifecycle**: `OnEngineStart`, `OnEngineUpdate`, `OnEngineFixedUpdate`, `OnEngineDestroy`, `OnEnginePause`
- **Sprite events**: `OnSpriteReady`, `OnSpriteUpdated`, `OnSpriteDestroyed`, `OnSpriteAnimationFinished`, `OnSpriteAnimationLooped`, `OnSpriteFrameChanged`, `OnSpriteVfxFinished`, `OnSpriteScreenEntered`, `OnSpriteScreenExited`
- **Input events**: `OnMousePressed`, `OnMouseReleased`, `OnKeyPressed`, `OnKeyReleased`, `OnActionPressed`, `OnActionJustPressed`, `OnActionJustReleased`, `OnAxisChanged`
- **Physics events**: `OnCollisionEnter`, `OnCollisionStay`, `OnCollisionExit`, `OnTriggerEnter`, `OnTriggerStay`, `OnTriggerExit`
- **UI events**: `OnUiReady`, `OnUiUpdated`, `OnUiDestroyed`, `OnUiPressed`, `OnUiReleased`, `OnUiHovered`, `OnUiClicked`, `OnUiToggle`

**Registration**:
Go code registers callbacks via `SpxEngine::register_callbacks(GDExtensionSpxCallbackInfoPtr)`, which are then invoked by the engine at appropriate times.

### Sprite System

**Three Sprite Types**:

1. **SpxSprite** (`spx_sprite.h/cpp`): Full-featured game sprite
   - Extends `CharacterBody2D` for physics
   - Implements `ISortableSprite` interface for layer sorting
   - Physics modes: `NO_PHYSICS`, `KINEMATIC`, `DYNAMIC`, `STATIC`
   - Contains: AnimatedSprite2D, Area2D (trigger), CollisionShape2D, VisibleOnScreenNotifier2D
   - Managed by SpxSpriteMgr

2. **SpxRenderSprite** (`spx_sprite.h`): Lightweight render-only sprite
   - Extends `Sprite2D`
   - No physics, best performance
   - Used for pure visual elements (backgrounds, decorations)
   - Created via `SpxSceneMgr::create_render_sprite()`

3. **SpxStaticSprite** (`spx_sprite.h`): Static physics sprite
   - Extends `StaticBody2D`
   - Has collision but doesn't move
   - Used for walls, platforms, obstacles
   - Created via `SpxSceneMgr::create_static_sprite()`

**Sortable Interface** (`ISortableSprite`):
All sprite types implement this for Y-sorting:
```cpp
virtual GdObj get_sort_id() const = 0;
virtual Point2 get_sort_position() const = 0;
virtual void set_sort_z_index(int z) = 0;
virtual int get_sort_z_index() const = 0;
virtual bool is_node_valid() const = 0;
virtual bool is_sort_static() const = 0;  // static vs dynamic sorting
```

## Layer Sorting System (SpxLayerSorter)

**CRITICAL**: This system manages sprite render order for pseudo-3D depth sorting.

**Location**: `modules/spx/spx_layer_sorter.h/cpp`

### Key Concepts

- **Static sprites**: Rarely move, sorted once and cached in `static_sorted` vector
- **Dynamic sprites**: Move frequently, incrementally re-sorted in `dynamic_sorted` vector
- **Incremental sorting**: Only re-sorts sprites that moved (marked dirty in `dynamic_dirty`)
- **Full sort threshold**: If >30% dirty (`full_sort_ratio`), full sort is faster than incremental
- **Sorting algorithm**: Y-first, then X (bottom-left sprites render behind top-right)

### Common Bug Pattern - DANGLING POINTERS

**THE MOST COMMON CAUSE OF SEGFAULTS IN THIS CODEBASE**:

```cpp
// WRONG: Can cause dangling pointers and segfaults
destroy_sprite(id);  // sprite freed, memory deallocated
// SpxLayerSorter still has ISortableSprite* pointing to freed memory
// Next frame: SEGFAULT when accessing sprite->get_sort_position()

// CORRECT: Clear layer sorter BEFORE destroying sprites
SpxLayerSorter::instance().reset();  // Clear all cached pointers
destroy_sprite(id);  // Now safe to free
```


### When to Call `SpxLayerSorter::instance().reset()`

**ALWAYS call reset() before**:
- Destroying any sprite (`destroy_sprite`, `destroy_pure_sprite`)
- Clearing scene (`clear_pure_sprites`, `destroy_all_sprites`)
- Unloading scene (`unload_current_scene`)
- Changing sprite physics mode (static ↔ dynamic conversion)
- Any operation that invalidates ISortableSprite pointers

**Why it's safe to call reset() frequently**:
- Just clears 4 vectors: `static_sorted`, `dynamic_sorted`, `dynamic_dirty`, `dynamic_dirty_ids`
- Sprites will be re-collected on next `update()` call
- Performance impact is minimal compared to avoiding crashes

### Debug Visualization

Enable with `Spx::set_debug_mode(true)` to create `LayerSorterDebugDrawer`:

**Visual indicators**:
- **Blue dots + "S:0", "S:1"**: Static sprites with sort order
- **Red dots + "D:0", "D:1"**: Dynamic sprites with sort order
- **Blue rectangle**: Camera viewport bounds
- **Z-index = 1000**: Debug drawer always renders on top

**Usage**:
```cpp
Spx::set_debug_mode(true);
// Now layer sorting is visualized in-game
```

### Sorting Flow

```cpp
void SpxSpriteMgr::on_update(float delta) {
    // 1. Collect all ISortableSprite pointers
    Vector<ISortableSprite*> sortables;
    for (auto sprite : all_sprites) {
        sortables.push_back(sprite);
    }

    // 2. Update layer sorter (handles sorting and z-index assignment)
    SpxLayerSorter::instance().update(sortables);
}

void SpxLayerSorter::update(const Vector<ISortableSprite*> &sortables) {
    _collect_sprites(sortables);      // Identify dirty sprites

    if (dirty_ratio > 0.3f) {
        _full_sort_dynamic();          // Sort entire dynamic_sorted
    } else {
        _incremental_sort_dynamic();   // Only re-insert dirty sprites
    }

    _apply_z_index_merged();          // Merge static + dynamic, set z_index
}
```

## Common Development Tasks

### Adding New Manager Functionality

1. **Add method to manager header** (e.g., `modules/spx/spx_sprite_mgr.h`):
```cpp
class SpxSpriteMgr : public SpxBaseMgr {
public:
    void new_feature(GdInt param);
};
```

2. **Implement in .cpp file** (`modules/spx/spx_sprite_mgr.cpp`):
```cpp
void SpxSpriteMgr::new_feature(GdInt param) {
    // Implementation
}
```

3. **If called from Go, add FFI export** (`modules/spx/gdextension_spx_ext.h`):
```cpp
typedef void (*GDExtensionSpxSpriteNewFeature)(GdInt param);
```

4. **Add to FFI registration** (`modules/spx/gdextension_spx_ext.cpp`):
```cpp
// In gdextension_spx_setup_interface()
```

5. **Update codegen** to generate Go bindings:
```bash
cd pkg/gdspx/cmd/codegen
go run .
```

6. **Regenerate all wrappers**:
```bash
cd /Users/tjp/projects/spx  # Root of spx project
make generate
```

### Adding New Callbacks

1. **Define callback type** (`modules/spx/gdextension_spx_ext.h`):
```cpp
typedef void (*GDExtensionSpxCallbackOnNewEvent)(GdObj obj, GdInt data);
```

2. **Add to SpxCallbackInfo struct** (in same file):
```cpp
typedef struct {
    // ... existing callbacks
    GDExtensionSpxCallbackOnNewEvent on_new_event;
} SpxCallbackInfo;
```

3. **Invoke in manager code** (e.g., `modules/spx/spx_sprite_mgr.cpp`):
```cpp
void SpxSpriteMgr::trigger_event(GdObj sprite_id, GdInt data) {
    auto callbacks = SpxEngine::get_singleton()->get_callbacks();
    if (callbacks && callbacks->on_new_event) {
        callbacks->on_new_event(sprite_id, data);
    }
}
```

4. **Update Go callback registration** (in `pkg/gdspx/internal/`):
Register the Go function to handle this callback.

### Debugging Segfaults

**Step 1: Identify if it's a dangling pointer**
- Most segfaults in modules/spx are dangling pointers in SpxLayerSorter
- Look for patterns: crash during rendering, crash after sprite destruction

**Step 2: Check recent sprite operations**
- Did code destroy a sprite without calling `SpxLayerSorter::instance().reset()`?
- Did code modify sprite positions without proper validation?

**Step 3: Add safety checks**
```cpp
// Before using ISortableSprite pointer:
if (sprite && sprite->is_node_valid()) {
    sprite->get_sort_position();  // Safe
}
```

**Step 4: Add reset() calls conservatively**
```cpp
// When in doubt, call reset() before sprite destruction
SpxLayerSorter::instance().reset();
destroy_sprite(id);
```

**Step 5: Enable debug mode**
```cpp
Spx::set_debug_mode(true);
// Watch for sprites that disappear but still show in debug visualization
```

### Memory Management Rules

**Godot objects**:
- Use `memnew(ClassName)` instead of `new ClassName`
- Use `memdelete(ptr)` instead of `delete ptr`
- Use `queue_free()` for nodes to remove from scene tree safely

**Reference-counted resources**:
```cpp
Ref<Texture2D> texture = load_texture(path);
// No manual deletion needed, ref-counted automatically
```

**Raw pointers in managers**:
- SpxLayerSorter stores raw `ISortableSprite*` pointers
- These become invalid when sprites are destroyed
- **Always call reset() before destroying sprites to clear these pointers**

## File Organization

### Naming Conventions

- `spx_*.h/cpp`: Manager implementations
- `spx.h/cpp`: Main static entry point
- `spx_engine.h/cpp`: Central singleton coordinator
- `gdextension_spx_ext.h/cpp`: FFI type definitions and function pointers
- `register_types.h/cpp`: Godot module registration boilerplate

### Header Structure

All files use Godot copyright header:
```cpp
/**************************************************************************/
/*  filename.h                                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
// ... MIT license ...
```

Include guards:
```cpp
#ifndef SPX_FEATURE_NAME_H
#define SPX_FEATURE_NAME_H
// ...
#endif // SPX_FEATURE_NAME_H
```

### Dependencies

**Manager headers include**:
- `spx_base_mgr.h` - Base class for all managers
- `gdextension_spx_ext.h` - FFI types (GdInt, GdString, GdObj, etc.)

**Implementation files include**:
- Godot scene graph headers: `scene/2d/*.h`, `scene/resources/*.h`
- Other spx managers as needed
- DO NOT expose Godot types to Go (use FFI types in headers)

## Build Outputs

After building with SCons, compiled object files appear in `modules/spx/`:

```
modules/spx/
├── spx_sprite.macos.editor.dev.arm64.o         # Editor build (debug)
├── spx_sprite.macos.template_release.arm64.o   # Runtime build (release)
├── spx_sprite.web.template_release.wasm32.o    # WebAssembly build
└── ... (one .o per .cpp file per platform/target)
```

Final binary:
- Desktop: `$GOPATH/bin/gdspxrt<version>` (macOS/Linux) or `gdspxrt<version>.exe` (Windows)
- Web: Exported as part of web template in `spx_web.zip`
- Mobile: APK (Android) or `.app` (iOS)

## Testing Changes

1. **Build modified engine**:
```bash
cd /Users/tjp/projects/spx
make build-desktop  # Rebuilds Godot + spx module
```

2. **Test with demo**:
```bash
make run DEMO_INDEX=3  # Run tutorial/03-Clone or whichever tests your feature
```

3. **Check for segfaults**:
- If crash occurs during sprite rendering → likely SpxLayerSorter dangling pointer
- If crash during startup → likely FFI callback issue
- If crash during destroy → likely missing `reset()` call

4. **Enable debug mode**:
```bash
# In your test game's main.spx or via code:
Spx::set_debug_mode(true)
```

## Related Documentation

- Main spx project: `/Users/tjp/projects/spx/CLAUDE.md`
- Godot documentation: https://docs.godotengine.org/
- SCons build system: See `SConstruct` in Godot root
- spx FFI layer: `pkg/gdspx/internal/` in main spx repository
