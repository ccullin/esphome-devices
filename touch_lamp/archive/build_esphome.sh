#!/bin/bash

# --- Configuration Variables ---
# Set your main ESPHome project directory (where your YAML and this script are)
ESPHOME_PROJECT_DIR="/Users/chriscullin/esphome-devices/touch_lamp"
# Set the name of your ESPHome YAML configuration file
ESPHOME_CONFIG_FILE="debug.yaml"
# Set the name of your ESPHome device (as defined in your YAML)
ESPHOME_DEVICE_NAME="debug"
# Path to your backup platformio.ini file
PLATFORMIO_BACKUP_FILE="${ESPHOME_PROJECT_DIR}/platformio.ini.backup"

# Optional: Set your upload port if auto-detection is unreliable
# UPLOAD_PORT="/dev/cu.usbserial-XXXXX" # Uncomment and set if needed
# Optional: Set a specific upload baud rate. 115200 is generally reliable.
UPLOAD_SPEED="115200"

# --- Script Logic ---

echo "--- Starting ESPHome Automated Build ---"

# 1. Go to the main ESPHome project directory
cd "${ESPHOME_PROJECT_DIR}" || { echo "Error: Could not change to project directory. Exiting."; exit 1; }
echo "Current directory: $(pwd)"

# 2. Verify that the platformio.ini.backup exists
if [ ! -f "${PLATFORMIO_BACKUP_FILE}" ]; then
    echo "Error: Backup PlatformIO.ini file not found at ${PLATFORMIO_BACKUP_FILE}"
    echo "Please create it first by following the CRUCIAL PRE-SCRIPT SETUP instructions."
    exit 1
fi

# Check for --fast argument
if [[ "$1" == "--fast" ]]; then
    echo "===== Phase 1: Skipping aggressive clean for fast compile (--fast option detected) ====="
    echo "=========================================================="
else
    echo "===== Phase 1: Performing Aggressive Clean ====="
    echo "=========================================================="

    # Clean ESPHome project build artifacts
    echo "Running esphome clean ${ESPHOME_CONFIG_FILE}..."
    esphome clean "${ESPHOME_CONFIG_FILE}"

    # Delete all PlatformIO packages and caches globally
    echo "Removing global PlatformIO packages and caches..."
    rm -rf ~/.platformio/packages/ || echo "Warning: Could not remove ~/.platformio/packages/"
    rm -rf ~/.platformio/platforms/ || echo "Warning: Could not remove ~/.platformio/platforms/"
    rm -rf ~/.platformio/penv/ || echo "Warning: Could not remove ~/.platformio/penv/"
    rm -rf ~/.platformio/.cache/ || echo "Warning: Could not remove ~/.platformio/.cache/"

    # Delete ESPHome's specific build directory for this device
    BUILD_DIR="${ESPHOME_PROJECT_DIR}/.esphome/build/${ESPHOME_DEVICE_NAME}"
    echo "Removing device-specific build directory: ${BUILD_DIR}..."
    rm -rf "${BUILD_DIR}" || echo "Warning: Could not remove ${BUILD_DIR}"

    # Delete ESPHome's own global cache
    echo "Removing global ESPHome cache..."
    rm -rf ~/.esphome/cache/ || echo "Warning: Could not remove ~/.esphome/cache/"

    echo "Clean complete."
fi

# 3. Run esphome compile to generate fresh platformio.ini and copy source files
# The BUILD_DIR needs to be set for subsequent steps, regardless of clean
BUILD_DIR="${ESPHOME_PROJECT_DIR}/.esphome/build/${ESPHOME_DEVICE_NAME}"

echo "===== Phase 2: Running esphome compile (expected to generate platformio.ini) ====="
echo "=========================================================="
echo "This step will likely fail, which is expected as it generates the problematic platformio.ini."
# Using '|| true' to prevent script from exiting if esphome compile fails
esphome compile "${ESPHOME_CONFIG_FILE}" || true

# 4. Copy the working platformio.ini.backup over the generated one
TARGET_PLATFORMIO_INI="${BUILD_DIR}/platformio.ini"
if [ -f "${TARGET_PLATFORMIO_INI}" ]; then
    echo "===== Phase 3: Overwriting generated platformio.ini with backup ====="
    echo "=========================================================="
    cp "${PLATFORMIO_BACKUP_FILE}" "${TARGET_PLATFORMIO_INI}"
    echo "Successfully copied ${PLATFORMIO_BACKUP_FILE} to ${TARGET_PLATFORMIO_INI}"
else
    echo "Error: Generated platformio.ini not found at ${TARGET_PLATFORMIO_INI}. Cannot proceed."
    exit 1
fi

# 5. Run platformio run from the build directory
echo "===== Phase 4: Running platformio run with the corrected platformio.ini ====="
echo "=========================================================="
cd "${BUILD_DIR}" || { echo "Error: Could not change to build directory. Exiting."; exit 1; }
echo "Current directory: $(pwd)"
platformio run

# 6. Copy the generated firmware.factory.bin to the project directory
echo "===== Phase 5: Copying firmware.factory.bin to project directory ====="
echo "=========================================================="
FIRMWARE_SOURCE_PATH="${BUILD_DIR}/.pio/build/${ESPHOME_DEVICE_NAME}/firmware.factory.bin"
FIRMWARE_DEST_PATH="${ESPHOME_PROJECT_DIR}/firmware_${ESPHOME_DEVICE_NAME}.bin"

if [ -f "${FIRMWARE_SOURCE_PATH}" ]; then
    cp "${FIRMWARE_SOURCE_PATH}" "${FIRMWARE_DEST_PATH}"
    echo "Firmware copied to: ${FIRMWARE_DEST_PATH}"
else
    echo "Warning: Firmware file not found at ${FIRMWARE_SOURCE_PATH}. Copy skipped."
fi
