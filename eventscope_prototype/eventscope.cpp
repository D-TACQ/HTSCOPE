#include <iostream>
#include <vector>
#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <iomanip>

std::vector<size_t> find_event_offsets(const uint8_t* data, size_t file_size, const uint8_t stride) {
    std::vector<size_t> offsets;
    
    // Example: replace this with 32-byte strided search 
    // and multi-threading logic later.
    const uint32_t MAGIC_NUM = 0xAA55F151;
    
    // A naive single-threaded scan for the prototype
    for (size_t i = 0; i + stride <= file_size; i += stride) {
        // Cast the current 32-byte boundary to a 32-bit integer pointer
        const uint32_t* current_val = reinterpret_cast<const uint32_t*>(data + i);
        
        if (*current_val == MAGIC_NUM) {
            offsets.push_back(i);
        }
    }
    
    return offsets;
}

int main(int argc, char* argv[]) {

    uint8_t STRIDE_ES_PLUS_SSB = 96;

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <test_file.bin>\n";
        return EXIT_FAILURE;
    }

    const char* filepath = argv[1];

    // start the setup timer
    auto setup_start = std::chrono::high_resolution_clock::now();

    // Open the file
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // Get the file size
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Error getting file size");
        close(fd);
        return EXIT_FAILURE;
    }
    size_t file_size = sb.st_size;

    if (file_size == 0) {
        std::cerr << "File is empty.\n";
        close(fd);
        return EXIT_SUCCESS;
    }

    // Memory map the file
    void* mapped_memory = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_memory == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return EXIT_FAILURE;
    }

    // Advise the kernel of sequential access
    if (posix_madvise(mapped_memory, file_size, POSIX_MADV_SEQUENTIAL) != 0) {
        std::cerr << "Warning: posix_madvise failed (performance may be sub-optimal)\n";
    }

    // Cast the mapped memory to a readable byte array
    const uint8_t* data = static_cast<const uint8_t*>(mapped_memory);
    
    // end the setup timer
    auto setup_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> setup_ms = setup_end - setup_start;


    std::cout << "--------------------------------------------------\n";
    std::cout << "File: " << filepath << " (" << (file_size / (1024.0 * 1024.0)) << " MB)\n";
    std::cout << "Setup & mmap time: " << setup_ms.count() << " ms\n";
    std::cout << "Scanning...\n";

    std::cout << "Successfully mapped " << filepath << " (" << file_size << " bytes)\n";
    std::cout << "Scanning for events...\n";

    // start scan timer
    auto scan_start = std::chrono::high_resolution_clock::now();

    // Call logic
    std::vector<size_t> found_offsets = find_event_offsets(data, file_size, STRIDE_ES_PLUS_SSB);

    // Print the results
    std::cout << "Found " << found_offsets.size() << " events.\n";
    for (size_t i = 0; i < found_offsets.size(); ++i) {
        std::cout << "  Event " << i + 1 << " at byte offset: " << found_offsets[i] << "\n";
    }
    
    // end scan timer
    auto scan_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> scan_sec = scan_end - scan_start;

    // calculate throughput in GB/s
    double file_size_gb = file_size / (1024.0 * 1024.0 * 1024.0);
    double throughput_gbps = file_size_gb / scan_sec.count();

    std::cout << "--------------------------------------------------\n";
    std::cout << "Found " << found_offsets.size() << " events.\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Scan execution time: " << scan_sec.count() << " seconds\n";
    
    // Only show GB/s if the file is reasonably large, otherwise it's noisy
    if (file_size > 1024 * 1024) {
        std::cout << "Effective Throughput:  " << throughput_gbps << " GB/s\n";
    }
    std::cout << "--------------------------------------------------\n";


    // Clean up
    if (munmap(mapped_memory, file_size) == -1) {
        perror("Error unmapping memory");
    }
    close(fd);

    return EXIT_SUCCESS;
}
