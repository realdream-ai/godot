#!/usr/bin/env python3
"""
Copy the spx_tilemap_exporter addon from pkg/gdspx/godot/addons to the current
project's addons directory, and use Godot to run the export script for automatic
spx_tilemap data export.

Godot Path:
    --godot: Command line argument to specify Godot executable path (highest priority)
    GODOT_PATH: Environment variable to specify Godot executable path
    
    If neither is specified, an error message will be displayed with instructions.
"""

import argparse
import os
import shutil
import subprocess
from pathlib import Path


def copy_addon(project_root: Path, script_dir: Path) -> bool:
    """Copy the spx_tilemap_exporter addon to the project's addons directory"""
    # Source path: pkg/gdspx/godot/addons/spx_tilemap_exporter
    src_path = project_root / "pkg" / "gdspx" / "godot" / "addons" / "spx_tilemap_exporter"
    
    # Destination path: tutorial/AA-00Town/addons/spx_tilemap_exporter
    dst_path = script_dir / "addons" / "spx_tilemap_exporter"
    
    # Check if source directory exists
    if not src_path.exists():
        print(f"Error: Source directory does not exist: {src_path}")
        return False
    
    # Ensure parent directory of destination exists
    dst_path.parent.mkdir(parents=True, exist_ok=True)
    
    # If destination directory already exists, remove it first
    if dst_path.exists():
        print(f"Removing existing directory: {dst_path}")
        shutil.rmtree(dst_path)
    
    # Copy directory
    print(f"Copying addon:")
    print(f"  Source: {src_path}")
    print(f"  Destination: {dst_path}")
    shutil.copytree(src_path, dst_path)
    print("Addon copy completed!")
    
    return True


# Global variable to store Godot path from command line argument
_godot_path_override: Path | None = None


def set_godot_path_override(path: str | None) -> None:
    """Set the Godot path override from command line argument"""
    global _godot_path_override
    if path:
        _godot_path_override = Path(path)
    else:
        _godot_path_override = None


def get_godot_path() -> Path | None:
    """Get the path to the Godot executable
    
    Priority:
    1. Command line argument (--godot)
    2. Environment variable (GODOT_PATH)
    3. Error message with instructions
    """
    global _godot_path_override
    
    # Priority 1: Command line argument
    if _godot_path_override:
        if not _godot_path_override.exists():
            print(f"Error: Godot executable does not exist: {_godot_path_override}")
            return None
        return _godot_path_override
    
    # Priority 2: Environment variable
    godot_path = os.environ.get("GODOT_PATH")
    if godot_path:
        godot_path = Path(godot_path)
        if not godot_path.exists():
            print(f"Error: Godot executable does not exist: {godot_path}")
            return None
        return godot_path
    
    # Priority 3: Error message
    print("Error: Godot path not specified")
    print("")
    print("Please specify the Godot executable path using one of the following methods:")
    print("  1. Command line argument: python export.py --godot /path/to/godot")
    print("  2. Environment variable:  export GODOT_PATH=/path/to/godot")
    print("                            set GODOT_PATH=C:\\path\\to\\godot.exe (Windows)")
    return None


def import_project(script_dir: Path) -> bool:
    """Use Godot to import the project (rescan and import resources)"""
    godot_path = get_godot_path()
    if not godot_path:
        return False
    
    # Build import command
    # godot --headless --path <project_path> --import
    cmd = [
        str(godot_path),
        "--headless",
        "--path", str(script_dir),
        "--import"
    ]
    
    print("")
    print("Importing project resources:")
    print(f"  Command: {' '.join(cmd)}")
    print("")
    
    # Execute command
    try:
        result = subprocess.run(cmd, cwd=str(script_dir))
        if result.returncode != 0:
            print(f"Error: Project import failed with return code: {result.returncode}")
            return False
        print("Project import completed!")
        return True
    except Exception as e:
        print(f"Error: Failed to execute Godot import: {e}")
        return False


def run_export(script_dir: Path, scene_path: str | None = None, enable_preview: bool = True) -> bool:
    """Use Godot to run the export script
    
    Args:
        script_dir: Path to the project directory
        scene_path: Optional scene path to export (e.g., "main.tscn" or "res://main.tscn")
                   If None, uses export_cli.gd's default (res://main.tscn)
                   The "res://" prefix is automatically added if not present.
        enable_preview: If True, run without --headless to enable preview PNG rendering.
                       Note: This will briefly show a Godot window.
    """
    godot_path = get_godot_path()
    if not godot_path:
        return False
    
    # Auto-add res:// prefix if not present
    if scene_path and not scene_path.startswith("res://"):
        scene_path = "res://" + scene_path
    
    # Build command
    # godot [--headless] --path <project_path> -s addons/spx_tilemap_exporter/export_cli.gd [-- --scene <path>]
    export_script = "addons/spx_tilemap_exporter/export_cli.gd"
    
    cmd = [str(godot_path)]
    
    # Only use headless mode when preview is disabled
    if not enable_preview:
        cmd.append("--headless")
    
    cmd.extend([
        "--path", str(script_dir),
        "-s", export_script
    ])
    
    # Add scene path argument if specified
    if scene_path:
        cmd.extend(["--", "--scene", scene_path])
    
    print("")
    print("Running export script:")
    print(f"  Command: {' '.join(cmd)}")
    if scene_path:
        print(f"  Scene: {scene_path}")
    else:
        print("  Scene: (using default: res://main.tscn)")
    if enable_preview:
        print("  Preview: enabled (default, non-headless mode)")
    else:
        print("  Preview: disabled (--no-preview, headless mode)")
    print("")
    
    # Execute command
    try:
        result = subprocess.run(cmd, cwd=str(script_dir))
        if result.returncode != 0:
            print(f"Error: Export script execution failed with return code: {result.returncode}")
            return False
        return True
    except Exception as e:
        print(f"Error: Failed to execute Godot: {e}")
        return False


def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description="SPX TileMap Export Tool - Export TileMap data from Godot project to SPX format.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Godot Path (one of the following is required):
  --godot       Command line argument to specify Godot executable path
  GODOT_PATH    Environment variable to specify Godot executable path
                
                Priority: --godot argument > GODOT_PATH environment variable
                
                Example: python export.py --godot /path/to/godot
                         export GODOT_PATH=/path/to/godot (Linux/macOS)
                         set GODOT_PATH=C:\\path\\to\\godot.exe (Windows)

Usage Examples:
  # Basic export with Godot path from environment variable
  python export.py

  # Specify Godot executable path via command line
  python export.py --godot /path/to/godot
  python export.py --godot C:\\path\\to\\godot.exe

  # Export with addon copy (copies spx_tilemap_exporter to project first)
  python export.py --copy

  # Export a specific scene (res:// prefix is optional)
  python export.py --scene levels/level1.tscn
  python export.py --scene res://levels/level1.tscn

  # Export without preview PNG (headless mode, no window)
  python export.py --no-preview
  python export.py --scene my_scene.tscn --no-preview

  # Combine multiple options (with preview disabled)
  python export.py --godot /path/to/godot --copy --scene my_scene.tscn --no-preview

Workflow:
  1. [Optional] Copy spx_tilemap_exporter addon to project (with --copy)
  2. Import project resources using Godot
  3. Run export script to generate SPX TileMap data

Note:
  This script should be placed in the target project directory
  (e.g., tutorial/AA-00Town/). It will automatically detect the
  project structure and export TileMap data accordingly.
"""
    )
    parser.add_argument(
        "--godot",
        type=str,
        default=None,
        metavar="PATH",
        help="Path to the Godot executable (overrides GODOT_PATH environment variable)"
    )
    parser.add_argument(
        "--copy",
        action="store_true",
        help="Copy spx_tilemap_exporter addon to project addons directory (not copied by default)"
    )
    parser.add_argument(
        "--scene",
        type=str,
        default=None,
        metavar="PATH",
        help="Path to the scene file to export, res:// prefix is optional (default: res://main.tscn)"
    )
    parser.add_argument(
        "--no-preview",
        action="store_true",
        dest="no_preview",
        help="Disable preview PNG export (runs Godot in headless mode, no window)"
    )
    args = parser.parse_args()
    
    # Set Godot path override from command line argument
    set_godot_path_override(args.godot)
    
    # Get the directory where this script is located (works regardless of where it's executed from)
    script_dir = Path(__file__).resolve().parent.parent.parent
    
    # Calculate project root directory (script is under tutorial/AA-00Town, go up two levels)
    project_root = script_dir.parent.parent
    
    # Determine number of steps based on whether copying is enabled
    total_steps = 3 if args.copy else 2
    current_step = 0
    
    print("=" * 50)
    print("SPX TileMap Export Tool")
    print("=" * 50)
    print("")
    
    # Step 1: Copy addon (only when --copy argument is specified)
    if args.copy:
        current_step += 1
        print(f"[Step {current_step}/{total_steps}] Copying spx_tilemap_exporter addon")
        print("-" * 50)
        if not copy_addon(project_root, script_dir):
            return 1
        print("")
    
    # Step 2: Import project (let Godot rescan and import resources)
    current_step += 1
    print(f"[Step {current_step}/{total_steps}] Importing project resources")
    print("-" * 50)
    if not import_project(script_dir):
        return 1
    
    print("")
    
    # Step 3: Run export script
    current_step += 1
    print(f"[Step {current_step}/{total_steps}] Running Godot export script")
    print("-" * 50)
    if not run_export(script_dir, args.scene, not args.no_preview):
        return 1
    
    print("")
    print("=" * 50)
    print("All done!")
    print("=" * 50)
    return 0


if __name__ == "__main__":
    exit(main())
