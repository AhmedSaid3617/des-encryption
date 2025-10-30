#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Configuration ---
EXECUTABLE="./a.out" # Assuming the executable is in the current directory
TEST_LOG="tests/test_log.txt"

# --- Determine Test Number ---
# If log file doesn't exist, create it
if [ ! -f "$TEST_LOG" ]; then
    echo "0" > "$TEST_LOG"
fi

# Get next test number
TEST_NUMBER=$(($(cat "$TEST_LOG") + 1))
echo "$TEST_NUMBER" > "$TEST_LOG"

# Create test directory
TEST_DIR="bins/$TEST_NUMBER"
mkdir -p "$TEST_DIR"
echo "Created test directory: $TEST_DIR"

# Set file paths within the test directory
DATA_FILE="$TEST_DIR/data.bin"
KEY_FILE="$TEST_DIR/key.bin"
ENCRYPTED_FILE="$TEST_DIR/encrypted.bin"
DECRYPTED_FILE="$TEST_DIR/decrypted.bin"

# --- File Generation ---

# 1. Create a large binary file with random content (1MB).
#    1MB = 1024 * 1024 bytes, which is a multiple of 8 bytes (64 bits).
echo "Generating a 1MB test file: $DATA_FILE"
dd if=/dev/urandom of="$DATA_FILE" bs=1024 count=1024 status=none

# 2. Create a file with a specific 64-bit (8-byte) key.
echo "Generating 64-bit key file: $KEY_FILE"
# Using a specific hex key: 0x0123456789ABCDEF
echo -ne "\x01\x23\x45\x67\x89\xab\xcd\xef" > "$KEY_FILE"

# 3. Ensure the output files exist and are empty.
echo "Creating empty files for output."
touch "$ENCRYPTED_FILE" "$DECRYPTED_FILE"

# --- Execution ---

# Check if the executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable not found at $EXECUTABLE"
    echo "Please build the project first."
    exit 1
fi

echo "Running encryption..."
"$EXECUTABLE" e "$KEY_FILE" "$DATA_FILE" "$ENCRYPTED_FILE"

echo "Running decryption..."
"$EXECUTABLE" d "$KEY_FILE" "$ENCRYPTED_FILE" "$DECRYPTED_FILE"

# --- Verification ---

echo "Comparing original file with decrypted file..."
if cmp -s "$DATA_FILE" "$DECRYPTED_FILE"; then
    echo "✅ Success: Files are identical."
else
    echo "❌ Failure: Files differ."
    exit 1
fi

echo "Test #$TEST_NUMBER completed successfully."
echo "Test files saved in: $TEST_DIR"