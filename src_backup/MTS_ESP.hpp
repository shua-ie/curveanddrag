#pragma once
#include <rack.hpp>
#include <string>
#include <atomic>
#include <mutex>

// Platform-specific includes for dynamic library loading
#ifdef _WIN32
    #include <windows.h>
    #define MTS_LIBRARY_NAME "libMTSClient.dll"
#elif defined(__APPLE__)
    #include <dlfcn.h>
    #define MTS_LIBRARY_NAME "libMTSClient.dylib"
#else
    #include <dlfcn.h>
    #define MTS_LIBRARY_NAME "libMTSClient.so"
#endif

namespace CurveAndDrag {

// Forward declarations matching the MTS-ESP API
typedef void* MTSClient;
typedef MTSClient (*MTS_RegisterClient_t)();
typedef void (*MTS_DeregisterClient_t)(MTSClient client);
typedef bool (*MTS_HasMaster_t)(MTSClient client);
typedef double (*MTS_NoteToFrequency_t)(MTSClient client, char midinote, char midichannel);
typedef double (*MTS_RetuningInSemitones_t)(MTSClient client, char midinote, char midichannel);
typedef bool (*MTS_ShouldFilterNote_t)(MTSClient client, char midinote, char midichannel);
typedef const char* (*MTS_GetScaleName_t)(MTSClient client);

/**
 * @brief MTS-ESP Client for VCV Rack integration
 * 
 * This class provides a thread-safe interface to the MTS-ESP microtuning system.
 * It dynamically loads the MTS-ESP library at runtime, allowing the plugin to
 * function even when MTS-ESP is not installed.
 * 
 * Features:
 * - Dynamic library loading with graceful fallback
 * - Thread-safe operation with proper mutex protection
 * - Automatic connection management
 * - Comprehensive error handling
 * - Memory leak prevention with RAII
 */
class MTSESPClient {
public:
    /**
     * @brief Constructor - initializes the MTS-ESP client
     */
    MTSESPClient() : 
        mtsClient(nullptr),
        libHandle(nullptr),
        libMtsLoaded(false),
        mtsConnected(false),
        mtsTuningName("Not Connected") {
        
        // Initialize function pointers to nullptr
        clearFunctionPointers();
        
        // Attempt to load MTS-ESP library
        tryLoadMtsEspLibrary();
    }

    /**
     * @brief Destructor - ensures proper cleanup
     */
    ~MTSESPClient() {
        cleanup();
    }

    // Disable copy constructor and assignment operator to prevent issues
    MTSESPClient(const MTSESPClient&) = delete;
    MTSESPClient& operator=(const MTSESPClient&) = delete;

    /**
     * @brief Attempt to dynamically load the MTS-ESP library
     * @return true if library was loaded successfully, false otherwise
     */
    bool tryLoadMtsEspLibrary() {
        std::lock_guard<std::mutex> lock(mtsMutex);
        
        if (libMtsLoaded) {
            return true; // Already loaded
        }
        
        // Platform-specific dynamic library loading
        #ifdef _WIN32
            libHandle = LoadLibrary(TEXT(MTS_LIBRARY_NAME));
            if (!libHandle) {
                DEBUG("MTS-ESP: Failed to load library %s", MTS_LIBRARY_NAME);
                return false;
            }
            
            // Load function pointers
            if (!loadWindowsFunctions()) {
                unloadLibrary();
                return false;
            }
        #else
            libHandle = dlopen(MTS_LIBRARY_NAME, RTLD_LAZY);
            if (!libHandle) {
                DEBUG("MTS-ESP: Failed to load library %s: %s", MTS_LIBRARY_NAME, dlerror());
                return false;
            }
            
            // Load function pointers
            if (!loadUnixFunctions()) {
                unloadLibrary();
                return false;
            }
        #endif
        
        libMtsLoaded = true;
        DEBUG("MTS-ESP: Library loaded successfully");
        
        // Register as client
        registerClient();
        
        return true;
    }

    /**
     * @brief Check if MTS-ESP master is available and update connection status
     * Should be called regularly from the audio thread
     */
    void pollForMtsConnection() {
        if (!libMtsLoaded || !mtsClient || !MTS_HasMaster) {
            return;
        }
        
        bool currentlyConnected = MTS_HasMaster(mtsClient);
        
        if (currentlyConnected != mtsConnected.load()) {
            std::lock_guard<std::mutex> lock(mtsMutex);
            mtsConnected = currentlyConnected;
            
            if (currentlyConnected) {
                updateTuningInfo();
                DEBUG("MTS-ESP: Connected to master");
            } else {
                mtsTuningName = "Not Connected";
                DEBUG("MTS-ESP: Disconnected from master");
            }
        }
    }

    /**
     * @brief Get frequency for a MIDI note using MTS-ESP tuning
     * @param midiNote MIDI note number (0-127)
     * @param midiChannel MIDI channel (0-15, or -1 for any channel)
     * @return Frequency in Hz, or standard 12-TET frequency if MTS not available
     */
    double getNoteFrequency(float midiNote, int midiChannel = -1) {
        if (!isMtsConnected() || !MTS_NoteToFrequency) {
            // Fallback to 12-TET
            return 440.0 * std::pow(2.0, (midiNote - 69.0) / 12.0);
        }
        
        char note = static_cast<char>(std::round(rack::math::clamp(midiNote, 0.0f, 127.0f)));
        char channel = static_cast<char>(rack::math::clamp(midiChannel, -1, 15));
        
        return MTS_NoteToFrequency(mtsClient, note, channel);
    }

    /**
     * @brief Get retuning amount in semitones for a MIDI note
     * @param midiNote MIDI note number (0-127)
     * @param midiChannel MIDI channel (0-15, or -1 for any channel)
     * @return Retuning amount in semitones
     */
    double getRetuningInSemitones(float midiNote, int midiChannel = -1) {
        if (!isMtsConnected() || !MTS_RetuningInSemitones) {
            return 0.0; // No retuning if MTS not available
        }
        
        char note = static_cast<char>(std::round(rack::math::clamp(midiNote, 0.0f, 127.0f)));
        char channel = static_cast<char>(rack::math::clamp(midiChannel, -1, 15));
        
        return MTS_RetuningInSemitones(mtsClient, note, channel);
    }

    /**
     * @brief Check if a note should be filtered (not played)
     * @param midiNote MIDI note number (0-127)
     * @param midiChannel MIDI channel (0-15, or -1 for any channel)
     * @return true if note should be filtered, false otherwise
     */
    bool shouldFilterNote(int midiNote, int midiChannel = -1) {
        if (!isMtsConnected() || !MTS_ShouldFilterNote) {
            return false; // Don't filter if MTS not available
        }
        
        char note = static_cast<char>(rack::math::clamp(midiNote, 0, 127));
        char channel = static_cast<char>(rack::math::clamp(midiChannel, -1, 15));
        
        return MTS_ShouldFilterNote(mtsClient, note, channel);
    }

    /**
     * @brief Check if MTS-ESP is currently connected
     * @return true if connected to MTS-ESP master, false otherwise
     */
    bool isMtsConnected() const {
        return mtsConnected.load();
    }

    /**
     * @brief Get the name of the current tuning scale
     * @return Scale name string
     */
    std::string getMtsTuningName() const {
        std::lock_guard<std::mutex> lock(mtsMutex);
        return mtsTuningName;
    }

    /**
     * @brief Check if MTS-ESP library is loaded
     * @return true if library is loaded, false otherwise
     */
    bool isLibraryLoaded() const {
        return libMtsLoaded;
    }

private:
    // MTS-ESP client and library handles
    MTSClient mtsClient;
    void* libHandle;
    
    // Function pointers
    MTS_RegisterClient_t MTS_RegisterClient;
    MTS_DeregisterClient_t MTS_DeregisterClient;
    MTS_HasMaster_t MTS_HasMaster;
    MTS_NoteToFrequency_t MTS_NoteToFrequency;
    MTS_RetuningInSemitones_t MTS_RetuningInSemitones;
    MTS_ShouldFilterNote_t MTS_ShouldFilterNote;
    MTS_GetScaleName_t MTS_GetScaleName;
    
    // State variables
    bool libMtsLoaded;
    std::atomic<bool> mtsConnected;
    mutable std::mutex mtsMutex;
    std::string mtsTuningName;

    /**
     * @brief Clear all function pointers to nullptr
     */
    void clearFunctionPointers() {
        MTS_RegisterClient = nullptr;
        MTS_DeregisterClient = nullptr;
        MTS_HasMaster = nullptr;
        MTS_NoteToFrequency = nullptr;
        MTS_RetuningInSemitones = nullptr;
        MTS_ShouldFilterNote = nullptr;
        MTS_GetScaleName = nullptr;
    }

    #ifdef _WIN32
    /**
     * @brief Load function pointers on Windows
     * @return true if all functions loaded successfully
     */
    bool loadWindowsFunctions() {
        MTS_RegisterClient = (MTS_RegisterClient_t)GetProcAddress((HMODULE)libHandle, "MTS_RegisterClient");
        MTS_DeregisterClient = (MTS_DeregisterClient_t)GetProcAddress((HMODULE)libHandle, "MTS_DeregisterClient");
        MTS_HasMaster = (MTS_HasMaster_t)GetProcAddress((HMODULE)libHandle, "MTS_HasMaster");
        MTS_NoteToFrequency = (MTS_NoteToFrequency_t)GetProcAddress((HMODULE)libHandle, "MTS_NoteToFrequency");
        MTS_RetuningInSemitones = (MTS_RetuningInSemitones_t)GetProcAddress((HMODULE)libHandle, "MTS_RetuningInSemitones");
        MTS_ShouldFilterNote = (MTS_ShouldFilterNote_t)GetProcAddress((HMODULE)libHandle, "MTS_ShouldFilterNote");
        MTS_GetScaleName = (MTS_GetScaleName_t)GetProcAddress((HMODULE)libHandle, "MTS_GetScaleName");
        
        return validateFunctionPointers();
    }
    #else
    /**
     * @brief Load function pointers on Unix systems
     * @return true if all functions loaded successfully
     */
    bool loadUnixFunctions() {
        MTS_RegisterClient = (MTS_RegisterClient_t)dlsym(libHandle, "MTS_RegisterClient");
        MTS_DeregisterClient = (MTS_DeregisterClient_t)dlsym(libHandle, "MTS_DeregisterClient");
        MTS_HasMaster = (MTS_HasMaster_t)dlsym(libHandle, "MTS_HasMaster");
        MTS_NoteToFrequency = (MTS_NoteToFrequency_t)dlsym(libHandle, "MTS_NoteToFrequency");
        MTS_RetuningInSemitones = (MTS_RetuningInSemitones_t)dlsym(libHandle, "MTS_RetuningInSemitones");
        MTS_ShouldFilterNote = (MTS_ShouldFilterNote_t)dlsym(libHandle, "MTS_ShouldFilterNote");
        MTS_GetScaleName = (MTS_GetScaleName_t)dlsym(libHandle, "MTS_GetScaleName");
        
        return validateFunctionPointers();
    }
    #endif

    /**
     * @brief Validate that all required function pointers were loaded
     * @return true if all required functions are available
     */
    bool validateFunctionPointers() {
        if (!MTS_RegisterClient || !MTS_DeregisterClient || !MTS_HasMaster || 
            !MTS_NoteToFrequency || !MTS_ShouldFilterNote) {
            DEBUG("MTS-ESP: Failed to load required function pointers");
            return false;
        }
        return true;
    }

    /**
     * @brief Register as MTS-ESP client
     */
    void registerClient() {
        if (!MTS_RegisterClient) return;
        
        mtsClient = MTS_RegisterClient();
        if (mtsClient) {
            DEBUG("MTS-ESP: Successfully registered as client");
        } else {
            DEBUG("MTS-ESP: Failed to register as client");
        }
    }

    /**
     * @brief Deregister MTS-ESP client
     */
    void deregisterClient() {
        if (mtsClient && MTS_DeregisterClient) {
            MTS_DeregisterClient(mtsClient);
            mtsClient = nullptr;
            DEBUG("MTS-ESP: Client deregistered");
        }
    }

    /**
     * @brief Update tuning information from MTS-ESP
     */
    void updateTuningInfo() {
        if (!MTS_GetScaleName) {
            mtsTuningName = "Connected";
            return;
        }
        
        const char* scaleName = MTS_GetScaleName(mtsClient);
        if (scaleName && strlen(scaleName) > 0) {
            mtsTuningName = std::string(scaleName);
        } else {
            mtsTuningName = "Connected";
        }
    }

    /**
     * @brief Unload the dynamic library
     */
    void unloadLibrary() {
        if (libHandle) {
            #ifdef _WIN32
                FreeLibrary((HMODULE)libHandle);
            #else
                dlclose(libHandle);
            #endif
            libHandle = nullptr;
        }
        clearFunctionPointers();
        libMtsLoaded = false;
    }

    /**
     * @brief Complete cleanup of all resources
     */
    void cleanup() {
        std::lock_guard<std::mutex> lock(mtsMutex);
        
        deregisterClient();
        unloadLibrary();
        
        mtsConnected = false;
        mtsTuningName = "Disconnected";
    }
};

} // namespace CurveAndDrag
