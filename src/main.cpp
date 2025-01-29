#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <vector>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#endif

#ifdef __APPLE__
std::string get_mount_point(const std::string& usb_path) {
    FILE* fp = popen("mount", "r");
    if (!fp) throw std::runtime_error("Failed to run mount command");

    char line[512];
    while(fgets(line, sizeof(line), fp)) {
        std::string entry(line);
        std::istringstream iss(entry);
        std::vector<std::string> parts;
        std::string part;

        while (iss >> part) parts.push_back(part);
        if (!parts.empty() && parts[0].find(usb_path) != std::string::npos) {
            pclose(fp);
            return parts[2];
        }
    }

    pclose(fp);
    throw std::runtime_error("Device is not mounted: " + usb_path);
}
#endif

void unmount_device(const std::string& usb_path) {
    #ifdef _WIN32
    HANDLE hVolume = CreateFile(
        usb_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hVolume == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open USB device: " + usb_path);
    }

    DWORD bytesReturned;
    if (!DeviceIoControl(hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL)) {
        CloseHandle(hVolume);
        throw std::runtime_error("Failed to unmount USB device: " + usb_path);
    }

    CloseHandle(hVolume);
    #elif defined(__APPLE__)
    std::string mount_point = get_mount_point(usb_path);

    if (unmount(mount_point.c_str(), MNT_FORCE) != 0) {
        perror("unmount failed");
        throw std::runtime_error("Failed to unmount USB device: " + usb_path);
    }
    #else
    if (umount(usb_path) != 0) {
        perror("umount failed");
        throw std::runtime_error("Failed to unmount USB device: " + usb_path);
    }
    #endif
}

void write_iso_to_usb(const std::string& iso_path, const std::string& usb_path) {
    std::ifstream iso_file(iso_path, std::ios::binary);
    if (!iso_file.is_open()) {
        throw std::runtime_error("Failed to open ISO file: " + iso_path);
    }

    #ifdef _WIN32
    HANDLE usb_device = CreateFile(
        usb_path.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (usb_device == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open USB device: " + usb_path);
    }
    #else
    int usb_device = open(usb_path.c_str(), O_WRONLY);
    if (usb_device < 0) {
        throw std::runtime_error("Failed to open USB device: " + usb_path + ", Error: " + strerror(errno));
    }
    #endif

    constexpr size_t buffer_size = 4096;
    char buffer[buffer_size];

    while (iso_file.read(buffer, buffer_size) || iso_file.gcount() > 0) {
        size_t bytes_to_write = iso_file.gcount();

        #ifdef _WIN32
        DWORD bytes_written;
        if (!WriteFile(usb_device, buffer, static_cast<DWORD>(bytes_to_write), &bytes_written, NULL) ||
            bytes_written != bytes_to_write) {
            CloseHandle(usb_device);
            throw std::runtime_error("Failed to write to USB device.");
        }
        #else
        ssize_t bytes_written = write(usb_device, buffer, bytes_to_write);
        if (bytes_written < 0) {
            close(usb_device);
            throw std::runtime_error("Failed to write to USB device: " + std::string(strerror(errno)));
        }
        #endif
    }

    iso_file.close();

    #ifdef _WIN32
    CloseHandle(usb_device);
    #else
    close(usb_device);
    #endif

    std::cout << "ISO file successfully written to USB stick." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ISO path> <USB device path>" << std::endl;
        return 1;
    }

    const std::string iso_path = argv[1];
    const std::string usb_path = argv[2];

    try {
        unmount_device(usb_path);

        write_iso_to_usb(iso_path, usb_path);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
