#pragma once

#ifdef PLATFORM_WINDOWS

#include <vector>
#include <string>
#include <memory>
#include <windows.h>

namespace SharedAudio {

    // ASIO driver information
    struct ASIODriverInfo {
        std::string name;
        std::string clsid;
        std::string driver_path;
        bool is_available;
        int version;
    };

    // Header-only ASIO interface (no COM dependencies)
    // Based on successful implementation from Syntri
    class ASIOHeaderInterface {
    public:
        // Detect ASIO drivers via registry (proven working in Syntri)
        static std::vector<ASIODriverInfo> detectASIODrivers() {
            std::vector<ASIODriverInfo> drivers;

            HKEY hKey;
            LONG result = RegOpenKeyExA(
                HKEY_LOCAL_MACHINE,
                "SOFTWARE\\ASIO",
                0,
                KEY_READ | KEY_WOW64_32KEY, // Important for 64-bit systems
                &hKey
            );

            if (result != ERROR_SUCCESS) {
                // Try 64-bit registry
                result = RegOpenKeyExA(
                    HKEY_LOCAL_MACHINE,
                    "SOFTWARE\\ASIO",
                    0,
                    KEY_READ | KEY_WOW64_64KEY,
                    &hKey
                );
            }

            if (result == ERROR_SUCCESS) {
                char keyName[256];
                DWORD keyIndex = 0;

                while (RegEnumKeyA(hKey, keyIndex++, keyName, sizeof(keyName)) == ERROR_SUCCESS) {
                    ASIODriverInfo info;
                    info.name = keyName;

                    // Get CLSID
                    HKEY hSubKey;
                    if (RegOpenKeyExA(hKey, keyName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                        char clsid[256] = { 0 };
                        DWORD dataSize = sizeof(clsid);
                        DWORD dataType;

                        if (RegQueryValueExA(hSubKey, "CLSID", nullptr, &dataType,
                            (LPBYTE)clsid, &dataSize) == ERROR_SUCCESS) {
                            info.clsid = clsid;
                        }

                        // Get driver description
                        char description[256] = { 0 };
                        dataSize = sizeof(description);
                        if (RegQueryValueExA(hSubKey, "Description", nullptr, &dataType,
                            (LPBYTE)description, &dataSize) == ERROR_SUCCESS) {
                            info.name = description; // Use description if available
                        }

                        RegCloseKey(hSubKey);
                    }

                    // Check if driver DLL exists
                    info.is_available = checkDriverAvailability(info.clsid);

                    drivers.push_back(info);
                }

                RegCloseKey(hKey);
            }

            return drivers;
        }

        // Initialize ASIO driver without COM (using JUCE's approach)
        static bool initializeASIODriver(const std::string& driver_name) {
            // This will be handled by JUCE's audio device manager
            // We just need to select the appropriate device
            return true;
        }

    private:
        static bool checkDriverAvailability(const std::string& clsid) {
            if (clsid.empty()) return false;

            // Check in registry for COM registration
            std::string regPath = "CLSID\\" + clsid + "\\InprocServer32";
            HKEY hKey;

            LONG result = RegOpenKeyExA(
                HKEY_CLASSES_ROOT,
                regPath.c_str(),
                0,
                KEY_READ,
                &hKey
            );

            if (result == ERROR_SUCCESS) {
                char dllPath[MAX_PATH] = { 0 };
                DWORD dataSize = sizeof(dllPath);
                DWORD dataType;

                bool exists = false;
                if (RegQueryValueExA(hKey, nullptr, nullptr, &dataType,
                    (LPBYTE)dllPath, &dataSize) == ERROR_SUCCESS) {
                    // Check if DLL file exists
                    exists = (GetFileAttributesA(dllPath) != INVALID_FILE_ATTRIBUTES);
                }

                RegCloseKey(hKey);
                return exists;
            }

            return false;
        }
    };

} // namespace SharedAudio

#endif // PLATFORM_WINDOWS