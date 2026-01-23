# SPX TileMap Exporter

A Godot editor plugin for exporting TileMap scenes and decorators to SPX JSON format for dynamic loading in the SPX runtime.

## Quick Start

```bash
# 1. Copy the plugin to your project's addons directory
cp -r spx_tilemap_exporter /path/to/your/project/addons/

# 2. Navigate to your project directory
cd /path/to/your/project

# 3. Export the scene
python addons/spx_tilemap_exporter/export.py --godot /path/to/godot --scene main.tscn

# Export files are saved in the `res://_export/<scene_name>/` directory
```

## Features

- **TileMap Export**: Export TileMapLayer, TileSet, and physics collision data
- **Decorator Export**: Export Sprite2D nodes and prefab instances from scenes
- **Preview PNG Export**: Export TileMapLayer content as a PNG preview image (Ctrl+Shift+E shortcut)
- **Automatic Texture Copy**: Automatically copy related textures to the export directory
- **Coordinate System Conversion**: Automatically convert Godot coordinates to SPX coordinates (Y-axis flip)
- **Collision Shape Support**: Support for exporting rectangle, circle, capsule, and polygon colliders
- **CLI Support**: Provides CLI scripts for batch automation exports

## Installation

1. Copy the `spx_tilemap_exporter` folder to the `addons/` directory of the Godot project you want to export

> **Note**: If you only use command-line export, the above steps complete the installation.
> To use the editor menu export feature, you also need to enable the "SPX TileMap Exporter" plugin in the Godot editor under **Project > Project Settings > Plugins**.

## Usage

### Command Line Export (Recommended)

Using the Python script for export is the simplest method:

**Method 1: Specify Godot path via command line argument**

```bash
python export.py --godot /path/to/godot --scene levels/level1.tscn
```

**Method 2: Specify Godot path via environment variable**

```bash
# Linux/macOS
export GODOT_PATH=/path/to/godot

# Windows
set GODOT_PATH=C:\path\to\godot.exe

# Export a specific scene
python export.py --scene levels/level1.tscn
```

**Godot Path Priority:**
1. `--godot` command line argument (highest priority)
2. `GODOT_PATH` environment variable
3. If neither is specified, an error message will be displayed

**Parameters:**

| Parameter | Description |
|-----------|-------------|
| `--godot PATH` | Specify the Godot executable path (takes priority over environment variable) |
| `--scene PATH` | Specify the scene file path to export, `res://` prefix is optional (default: `res://main.tscn`) |
| `--no-preview` | Disable preview PNG export (runs Godot in headless mode, no window). By default, preview is enabled. |


### Editor Menu Export

> **Prerequisite**: You need to enable the plugin in the Godot editor first (**Project > Project Settings > Plugins**)

After enabling the plugin, you can access export features through the **Project > Tools** menu:

| Menu Item | Function |
|-----------|----------|
| **SPX Export TileMap...** | Export only the TileMap data from the current scene |
| **SPX Export Decorators...** | Export only the decorator data from the current scene |
| **SPX Export All...** | Export both TileMap and decorator data |
| **SPX Export Preview PNG (Ctrl+Shift+E)** | Export the scene as a PNG preview image |

### Keyboard Shortcut

| Shortcut | Function |
|----------|----------|
| **Ctrl+Shift+E** | Quickly export the current scene as a PNG preview image |

The shortcut works when editing 2D nodes in the canvas editor. The exported preview image includes only TileMapLayer content (Sprite2D and other nodes are excluded).


### Direct Godot CLI Usage

For more granular control, you can use the Godot command line directly:

```bash
# Basic usage (exports default scene res://main.tscn)
godot --headless --path <project_path> -s addons/spx_tilemap_exporter/export_cli.gd

# Export a specific scene
godot --headless --path <project_path> -s addons/spx_tilemap_exporter/export_cli.gd -- --scene res://levels/level1.tscn
```

| Parameter | Description |
|-----------|-------------|
| `--scene <path>` | Specify the scene path to export (must be placed after `--`) |

## Export Output

Export files are saved in the `res://_export/<scene_name>/` directory:

```
_export/
└── <scene_name>/
    ├── tilemap.json        # TileMap data
    ├── tilemap/            # TileMap texture directory
    │   └── *.png
    ├── decorator.json      # Decorator data
    ├── decorator/          # Decorator texture directory
    │   └── *.png
    └── preview.png         # Scene preview image
```

## Configuration

### CLI Script Configuration

Edit the constants in `export_cli.gd` to modify the default configuration:

```gdscript
const DEFAULT_SCENE_PATH = "res://main.tscn"  # Default scene path to export
const EXPORT_TILEMAP = true                   # Whether to export TileMap
const EXPORT_DECORATORS = true                # Whether to export decorators
const EXPORT_PREVIEW = true                   # Whether to export preview PNG
const PREVIEW_RENDER_DELAY = 0.5              # Delay for viewport rendering (seconds)
```

> **Note**: Preview PNG export requires rendering, which may not work properly in `--headless` mode on systems without a GPU. If preview export fails in headless mode, you can disable it by setting `EXPORT_PREVIEW = false`.

### Excluding Nodes

The following nodes are automatically excluded during decorator export:

- Nodes belonging to the `spx_ignore` group
- TileMapLayer and TileMap nodes (exported separately)
- Nodes with names containing `_ignore` or `_skip`

## File Descriptions

| File | Description |
|------|-------------|
| `plugin.cfg` | Plugin configuration file |
| `spx_tilemap_exporter.gd` | Main plugin script, provides editor menu functionality |
| `tilemap_extractor.gd` | TileMap data extraction and export logic |
| `decorator_extractor.gd` | Decorator data extraction and export logic |
| `preview_exporter.gd` | Scene preview PNG export logic (TileMapLayer only, provides static utility functions) |
| `export_cli.gd` | Command-line export script (Godot --headless mode) |
| `export.py` | Python automation export script |


## Requirements

- Godot 4.x
- Python 3.8+ (when using the Python export script)

---

## For Engine Developers

> The following content is for SPX engine developers only. Regular users can skip this section.

### Plugin Rapid Iteration

When developing the `spx_tilemap_exporter` plugin, you can use the `--copy` parameter to copy the latest plugin code from the engine source directory to the target project:

```bash
# Export with addon copy (copy latest plugin from source to project directory)
python export.py --copy

# Combined usage: copy plugin and export a specific scene
python export.py --copy --scene my_scene.tscn

# Combined usage: specify Godot path, copy plugin, and export a specific scene
python export.py --godot /path/to/godot --copy --scene my_scene.tscn
```

| Parameter | Description |
|-----------|-------------|
| `--godot PATH` | Specify the Godot executable path (takes priority over `GODOT_PATH` environment variable) |
| `--copy` | Copy the latest plugin from `pkg/gdspx/godot/addons/` to the current project directory for rapid plugin development iteration |

**Workflow:**

1. Modify the plugin code in `pkg/gdspx/godot/addons/spx_tilemap_exporter/`
2. Navigate to the test project directory (e.g., `tutorial/AA-00Town/`)
3. Run `python export.py --copy` to automatically copy the plugin and test the export

## License

SPX Team © 2026
