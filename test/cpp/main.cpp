#include <iostream>
#include <vector>
#include <fstream>
#include <windows.h>
#include <cstdint>

// Function pointer types for the DLL exports
typedef void* (*PocketTtsLoadModelFn)(const char*, const char*);
typedef void (*PocketTtsFreeModelFn)(void*);
typedef void* (*PocketTtsGetVoiceStateFn)(void*, const char*);
typedef void (*PocketTtsFreeVoiceStateFn)(void*);
typedef float* (*PocketTtsGenerateFn)(void*, const char*, void*, size_t*);
typedef void (*PocketTtsFreeAudioFn)(float*, size_t);

// Simple WAV header structure
#pragma pack(push, 1)
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 3; // IEEE Float
    uint16_t numChannels = 1;
    uint32_t sampleRate = 24000;
    uint32_t byteRate = 24000 * 4;
    uint16_t blockAlign = 4;
    uint16_t bitsPerSample = 32;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;
};
#pragma pack(pop)

void save_wav(const char* filename, float* audio_data, size_t num_samples) {
    WavHeader header;
    header.dataSize = num_samples * sizeof(float);
    header.fileSize = 36 + header.dataSize;

    std::ofstream outfile(filename, std::ios::binary);
    outfile.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader));
    outfile.write(reinterpret_cast<const char*>(audio_data), header.dataSize);
    outfile.close();
}

int main() {
    // Load the DLL
    HMODULE hDll = LoadLibraryA("pocket_tts_ffi.dll");
    if (!hDll) {
        std::cerr << "Failed to load pocket_tts_ffi.dll (Error: " << GetLastError() << ")" << std::endl;
        return 1;
    }

    // Get function pointers
    auto load_model = (PocketTtsLoadModelFn)GetProcAddress(hDll, "pocket_tts_load_model");
    auto free_model = (PocketTtsFreeModelFn)GetProcAddress(hDll, "pocket_tts_free_model");
    auto get_voice_state = (PocketTtsGetVoiceStateFn)GetProcAddress(hDll, "pocket_tts_get_voice_state");
    auto free_voice_state = (PocketTtsFreeVoiceStateFn)GetProcAddress(hDll, "pocket_tts_free_voice_state");
    auto generate = (PocketTtsGenerateFn)GetProcAddress(hDll, "pocket_tts_generate");
    auto free_audio = (PocketTtsFreeAudioFn)GetProcAddress(hDll, "pocket_tts_free_audio");

    if (!load_model || !free_model || !get_voice_state || !free_voice_state || !generate || !free_audio) {
        std::cerr << "Failed to find exported functions in DLL" << std::endl;
        return 1;
    }

    std::cout << "Loading model..." << std::endl;
    // Provide the path where models are stored
    const char* model_dir = "D:/Code/mebrain-figures/models/tts";
    void* model = load_model(model_dir, "b6369a24");
    if (!model) {
        std::cerr << "Failed to load model" << std::endl;
        return 1;
    }

    std::cout << "Model loaded. Getting voice state..." << std::endl;
    // Paths are relative to the execution directory (test/cpp/build)
    void* voice_state = get_voice_state(model, "../../../assets/ref.wav");
    if (!voice_state) {
        std::cerr << "Failed to get voice state (is assets/ref.wav present?)" << std::endl;
        free_model(model);
        return 1;
    }

    std::cout << "Voice state acquired. Generating audio..." << std::endl;
    size_t out_len = 0;
    float* audio_data = generate(model, "Hello from C++ Windows DLL into a WAVE file!", voice_state, &out_len);

    if (audio_data) {
        std::cout << "Generated " << out_len << " samples!" << std::endl;
        
        save_wav("output_test.wav", audio_data, out_len);
        std::cout << "Saved samples to output_test.wav" << std::endl;

        free_audio(audio_data, out_len);
    } else {
        std::cerr << "Failed to generate audio" << std::endl;
    }

    std::cout << "Cleaning up..." << std::endl;
    free_voice_state(voice_state);
    free_model(model);
    FreeLibrary(hDll);

    return 0;
}
