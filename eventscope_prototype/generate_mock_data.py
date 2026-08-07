import os
import struct
import sys

# --- Configuration ---
MAGIC_NUM = 0xAA55F151
SAMPLE_SIZE = 64
EVENT_SIG_SIZE = 32

# For testing, we'll pretend the pre/post window is 1 MB instead of 256 MB
TEST_WINDOW_MB = 1
SAMPLES_PER_MB = (TEST_WINDOW_MB * 1024 * 1024) // SAMPLE_SIZE

try:
    size_multiplier = int(sys.argv[1])
except:
    size_multiplier = 1

# preallocate a 1MB noise block
CHUNK_SIZE = 1024 * 1024
NOISE_CHUNK = os.urandom(CHUNK_SIZE)


def write_samples(f, num_samples):
    """Writes N contiguous 64-byte samples of random data."""
    # os.urandom is fast and simulates the noise of 32x16-bit analog channels
    total_bytes = num_samples * SAMPLE_SIZE
    bytes_written = 0

    while bytes_written < total_bytes:
        bytes_to_write = min(CHUNK_SIZE, total_bytes - bytes_written)
        f.write(NOISE_CHUNK[:bytes_to_write])
        bytes_written += bytes_to_write


def write_event(f):
    """Writes a 32-byte event signature."""
    # '<I' packs the integer as Little-Endian Unsigned 32-bit
    magic_bytes = struct.pack('<I', MAGIC_NUM)
    padding = os.urandom(EVENT_SIG_SIZE - 4)  # 28 bytes of junk data
    f.write(magic_bytes + padding)


# File 1: No Event Signatures (Baseline)
print("Generating: test_01_no_events.bin")
with open("test_01_no_events.bin", "wb") as f:
    # Write 10 MB of pure sample data
    write_samples(f, SAMPLES_PER_MB * 10 * size_multiplier)


# File 2: Nicely Spaced Events (No Overlap)
print("Generating: test_02_spaced_events.bin")
with open("test_02_spaced_events.bin", "wb") as f:
    write_samples(f, SAMPLES_PER_MB * 3 * size_multiplier)   # 3 MB of samples
    write_event(f)                         # EVENT 1
    write_samples(f, SAMPLES_PER_MB * 4 * size_multiplier)   # 4 MB gap (> 2MB window, no overlap)
    write_event(f)                         # EVENT 2
    write_samples(f, SAMPLES_PER_MB * 3 * size_multiplier)   # 3 MB of samples


# File 3: Closely Spaced Events (Overlap)
print("Generating: test_03_overlap_events.bin")
with open("test_03_overlap_events.bin", "wb") as f:
    write_samples(f, SAMPLES_PER_MB * 3 * size_multiplier)   # 3 MB of samples
    write_event(f)                         # EVENT 1
    write_samples(f, SAMPLES_PER_MB // 2)  # 0.5 MB gap (Overlaps the 1MB pre/post windows!)
    write_event(f)                         # EVENT 2
    write_samples(f, SAMPLES_PER_MB * 3 * size_multiplier)   # 3 MB of samples

print("Done. Test files created.")
