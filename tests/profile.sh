#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Configuration ---
EXECUTABLE="./a.out" # Assuming the executable is in the current directory
LOG_FILE="profiling_results.txt"
MAIN_TEST_DIR="bins/main_test"

# Check if main_test directory exists
if [ ! -d "$MAIN_TEST_DIR" ]; then
    echo "Error: Main test directory not found at $MAIN_TEST_DIR"
    echo "Please create the directory and add test files first."
    exit 1
fi

# Set file paths from main_test directory (all files in bins/main_test)
DATA_FILE="$MAIN_TEST_DIR/data.bin"
KEY_FILE="$MAIN_TEST_DIR/key.bin"
ENCRYPTED_FILE="$MAIN_TEST_DIR/encrypted.bin"
DECRYPTED_FILE="$MAIN_TEST_DIR/decrypted.bin"

# Check if required input files exist
if [ ! -f "$DATA_FILE" ]; then
    echo "Error: Data file not found at $DATA_FILE"
    exit 1
fi

if [ ! -f "$KEY_FILE" ]; then
    echo "Error: Key file not found at $KEY_FILE"
    exit 1
fi

# --- Initialize Log File ---
# If log file doesn't exist, create it with headers
if [ ! -f "$LOG_FILE" ]; then
    echo "Test_Number,Timestamp,Encryption_Time(s),Decryption_Time(s),Encryption_Throughput(MB/s),Decryption_Throughput(MB/s)" > "$LOG_FILE"
fi

# Get next test number
if [ -f "$LOG_FILE" ] && [ "$(wc -l < "$LOG_FILE")" -gt 1 ]; then
    TEST_NUMBER=$(($(tail -1 "$LOG_FILE" | cut -d',' -f1) + 1))
else
    TEST_NUMBER=1
fi

# Get current timestamp
TIMESTAMP=$(date "+%Y-%m-%d %H:%M:%S")

echo "Using test files from: $MAIN_TEST_DIR"
echo "Data file: $DATA_FILE"
echo "Key file: $KEY_FILE"

# Get file size in bytes
FILE_SIZE_BYTES=$(stat -c%s "$DATA_FILE")
# Convert to MB
FILE_SIZE_MB=$(echo "scale=6; $FILE_SIZE_BYTES / 1048576" | bc)
echo "File size: $FILE_SIZE_MB MB"

# --- Execution ---

# Check if the executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable not found at $EXECUTABLE"
    echo "Please build the project first."
    exit 1
fi

echo "Running encryption..."
START_TIME=$(date +%s.%N)
"$EXECUTABLE" e "$KEY_FILE" "$DATA_FILE" "$ENCRYPTED_FILE"
END_TIME=$(date +%s.%N)
ENCRYPTION_TIME=$(echo "$END_TIME - $START_TIME" | bc)
ENCRYPTION_THROUGHPUT=$(echo "scale=6; $FILE_SIZE_MB / $ENCRYPTION_TIME" | bc)
echo "Encryption completed in ${ENCRYPTION_TIME}s (${ENCRYPTION_THROUGHPUT} MB/s)"

echo "Running decryption..."
START_TIME=$(date +%s.%N)
"$EXECUTABLE" d "$KEY_FILE" "$ENCRYPTED_FILE" "$DECRYPTED_FILE"
END_TIME=$(date +%s.%N)
DECRYPTION_TIME=$(echo "$END_TIME - $START_TIME" | bc)
DECRYPTION_THROUGHPUT=$(echo "scale=6; $FILE_SIZE_MB / $DECRYPTION_TIME" | bc)
echo "Decryption completed in ${DECRYPTION_TIME}s (${DECRYPTION_THROUGHPUT} MB/s)"

# --- Verification ---

echo "Comparing original file with decrypted file..."
if cmp -s "$DATA_FILE" "$DECRYPTED_FILE"; then
    echo "✅ Success: Files are identical."
else
    echo "❌ Failure: Files differ."
    exit 1
fi

# --- Log Profiling Results ---
echo "Logging profiling results to $LOG_FILE"
echo "$TEST_NUMBER,$TIMESTAMP,$ENCRYPTION_TIME,$DECRYPTION_TIME,$ENCRYPTION_THROUGHPUT,$DECRYPTION_THROUGHPUT" >> "$LOG_FILE"
printf "Test #%-5s | Encryption: %-10s (%-10s MB/s) | Decryption: %-10s (%-10s MB/s)\n" \
    "$TEST_NUMBER" "${ENCRYPTION_TIME}s" "$ENCRYPTION_THROUGHPUT" "${DECRYPTION_TIME}s" "$DECRYPTION_THROUGHPUT"

echo "Profiling completed successfully."
