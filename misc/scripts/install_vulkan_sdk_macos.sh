#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

echo "Starting Vulkan SDK installation..."

# Specify installation directory
INSTALL_DIR="$HOME/VulkanSDK"

# Check if already installed
if [ -d "$INSTALL_DIR" ]; then
    echo "Detected Vulkan SDK already installed at $INSTALL_DIR"
    SDK_VERSION=$(ls -1 "$INSTALL_DIR" | sort -V | tail -1)
    echo "Current installed version: $SDK_VERSION"
    
    # Check environment variables
    if ! grep -q "VULKAN_SDK" ~/.zshrc; then
        echo "VULKAN_SDK environment variable not found, adding it..."
        echo "export VULKAN_SDK=\"$INSTALL_DIR/$SDK_VERSION\"" >> ~/.zshrc
        echo "Environment variable added to ~/.zshrc, please run 'source ~/.zshrc' to apply."
    else
        echo "VULKAN_SDK environment variable is already set."
    fi
    
    echo "Vulkan SDK is already installed. Skipping installation."
    echo "You can now build Godot with 'scons platform=macos vulkan=yes'."
    exit 0
fi

# If we get here, Vulkan SDK is not installed yet, so proceed with installation

# Clean up old files
rm -rf /tmp/vulkan-sdk.dmg /tmp/vulkan-sdk-extracted

# Download Vulkan SDK (which is actually a ZIP file)
echo "Downloading Vulkan SDK..."
curl -L "https://sdk.lunarg.com/sdk/download/latest/mac/vulkan-sdk.dmg" -o /tmp/vulkan-sdk.dmg

# Create extraction directory
mkdir -p /tmp/vulkan-sdk-extracted

# Extract the file as ZIP
echo "Extracting Vulkan SDK..."
unzip -q /tmp/vulkan-sdk.dmg -d /tmp/vulkan-sdk-extracted

# Find the installer app
INSTALLER_APP=$(find /tmp/vulkan-sdk-extracted -maxdepth 1 -name "*.app" | head -1)
echo "Found installer: $INSTALLER_APP"

# Find the installer executable
INSTALLER_BIN=$(find "$INSTALLER_APP/Contents/MacOS" -type f -name "*ulkan*" | head -1)
echo "Found installer executable: $INSTALLER_BIN"

# Run the installer
echo "Running installer..."
"$INSTALLER_BIN" --accept-licenses --default-answer --confirm-command install

# Clean up temporary files
rm -rf /tmp/vulkan-sdk.dmg /tmp/vulkan-sdk-extracted

# Get installed version
SDK_VERSION=$(ls -1 "$INSTALL_DIR" | sort -V | tail -1)
echo "Vulkan SDK version $SDK_VERSION installed to $INSTALL_DIR/$SDK_VERSION"

# Set environment variable
if ! grep -q "VULKAN_SDK" ~/.zshrc; then
    echo "Setting VULKAN_SDK environment variable..."
    echo "export VULKAN_SDK=\"$INSTALL_DIR/$SDK_VERSION\"" >> ~/.zshrc
    echo "Environment variable added to ~/.zshrc, please run 'source ~/.zshrc' to apply."
else
    echo "VULKAN_SDK environment variable already exists, you may need to manually update it to new version path: $INSTALL_DIR/$SDK_VERSION"
fi

echo "Vulkan SDK installation successful! You can now run 'source ~/.zshrc' to update environment variables, then build Godot with 'scons platform=macos vulkan=yes'."