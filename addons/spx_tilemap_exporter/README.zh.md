# SPX TileMap Exporter

Godot 编辑器插件，用于将 TileMap 场景和装饰器导出为 SPX JSON 格式，以便在 SPX 运行时动态加载。

## 快速开始

```bash
# 1. 将插件复制到项目的 addons 目录
cp -r spx_tilemap_exporter /path/to/your/project/addons/

# 2. 进入项目目录
cd /path/to/your/project

# 3. 导出场景
python addons/spx_tilemap_exporter/export.py --godot /path/to/godot --scene main.tscn

# 导出文件保存在 `res://_export/<场景名>/` 目录：
```

## 功能特性

- **TileMap 导出**：导出 TileMapLayer、TileSet 及物理碰撞数据
- **装饰器导出**：导出场景中的 Sprite2D 节点和预制体实例
- **纹理自动复制**：自动将相关纹理复制到导出目录
- **坐标系转换**：自动将 Godot 坐标系转换为 SPX 坐标系（Y 轴翻转）
- **碰撞形状支持**：支持导出矩形、圆形、胶囊体和多边形碰撞器
- **命令行支持**：提供 CLI 脚本用于批量自动化导出

## 安装

1. 将 `spx_tilemap_exporter` 文件夹复制到需要导出的 Godot 项目的 `addons/` 目录

> **注意**：如果只使用命令行导出，以上步骤即可完成安装。
> 如需使用编辑器菜单导出功能，还需在 Godot 编辑器中 **项目 > 项目设置 > 插件** 启用 "SPX TileMap Exporter" 插件。

## 使用方法

### 命令行导出（推荐）

使用 Python 脚本进行导出是最简单的方式：

**方式一：通过命令行参数指定 Godot 路径**

```bash
python export.py --godot /path/to/godot --scene levels/level1.tscn
```

**方式二：通过环境变量指定 Godot 路径**

```bash
# Linux/macOS
export GODOT_PATH=/path/to/godot

# Windows
set GODOT_PATH=C:\path\to\godot.exe

# 导出指定场景
python export.py --scene levels/level1.tscn
```

**Godot 路径优先级：**
1. `--godot` 命令行参数（最高优先级）
2. `GODOT_PATH` 环境变量
3. 如果都未指定，将显示错误提示

**参数说明：**

| 参数 | 说明 |
|------|------|
| `--godot PATH` | 指定 Godot 可执行文件路径（优先级高于环境变量） |
| `--scene PATH` | 指定要导出的场景文件路径，`res://` 前缀可选（默认：`res://main.tscn`） |


### 编辑器菜单导出

> **前提条件**：需要先在 Godot 编辑器中启用插件（**项目 > 项目设置 > 插件**）

启用插件后，可通过 **项目 > 工具** 菜单访问导出功能：

| 菜单项 | 功能 |
|--------|------|
| **SPX Export TileMap...** | 仅导出当前场景的 TileMap 数据 |
| **SPX Export Decorators...** | 仅导出当前场景的装饰器数据 |
| **SPX Export All...** | 同时导出 TileMap 和装饰器数据 |


### 直接使用 Godot CLI

如果需要更精细的控制，也可以直接使用 Godot 命令行：

```bash
# 基础用法（导出默认场景 res://main.tscn）
godot --headless --path <项目路径> -s addons/spx_tilemap_exporter/export_cli.gd

# 导出指定场景
godot --headless --path <项目路径> -s addons/spx_tilemap_exporter/export_cli.gd -- --scene res://levels/level1.tscn
```

| 参数 | 说明 |
|------|------|
| `--scene <path>` | 指定要导出的场景路径（必须放在 `--` 之后） |

## 导出输出

导出文件保存在 `res://_export/<场景名>/` 目录：

```
_export/
└── <场景名>/
    ├── tilemap.json        # TileMap 数据
    ├── tilemap/            # TileMap 纹理目录
    │   └── *.png
    ├── decorator.json      # 装饰器数据
    └── decorator/          # 装饰器纹理目录
        └── *.png
```

## 配置

### CLI 脚本配置

编辑 `export_cli.gd` 中的常量来修改默认配置：

```gdscript
const DEFAULT_SCENE_PATH = "res://main.tscn"  # 默认导出的场景路径
const EXPORT_TILEMAP = true                   # 是否导出 TileMap
const EXPORT_DECORATORS = true                # 是否导出装饰器
```

### 排除节点

装饰器导出时会自动排除以下节点：

- 属于 `spx_ignore` 组的节点
- TileMapLayer 和 TileMap 节点（单独导出）
- 名称包含 `_ignore` 或 `_skip` 的节点

## 文件说明

| 文件 | 描述 |
|------|------|
| `plugin.cfg` | 插件配置文件 |
| `spx_tilemap_exporter.gd` | 主插件脚本，提供编辑器菜单功能 |
| `tilemap_extractor.gd` | TileMap 数据提取和导出逻辑 |
| `decorator_extractor.gd` | 装饰器数据提取和导出逻辑 |
| `export_cli.gd` | 命令行导出脚本（Godot --headless 模式） |
| `export.py` | Python 自动化导出脚本 |


## 环境要求

- Godot 4.x
- Python 3.8+（使用 Python 导出脚本时）

---

## 引擎开发者

> 以下内容仅供 SPX 引擎开发者参考，普通用户可忽略此章节。

### 插件快速迭代

在开发 `spx_tilemap_exporter` 插件时，可以使用 `--copy` 参数将最新的插件代码从引擎源码目录复制到目标项目：

```bash
# 带 addon 复制的导出（从源码复制最新插件到项目目录）
python export.py --copy

# 组合使用：复制插件并导出指定场景
python export.py --copy --scene my_scene.tscn

# 组合使用：指定 Godot 路径、复制插件并导出指定场景
python export.py --godot /path/to/godot --copy --scene my_scene.tscn
```

| 参数 | 说明 |
|------|------|
| `--godot PATH` | 指定 Godot 可执行文件路径（优先级高于 `GODOT_PATH` 环境变量） |
| `--copy` | 从 `pkg/gdspx/godot/addons/` 复制最新的插件到当前项目目录，用于快速迭代插件开发 |

**工作流程：**

1. 在 `pkg/gdspx/godot/addons/spx_tilemap_exporter/` 修改插件代码
2. 进入测试项目目录（如 `tutorial/AA-00Town/`）
3. 运行 `python export.py --copy` 自动复制插件并测试导出

## 许可证

SPX Team © 2026
