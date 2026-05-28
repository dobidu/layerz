// Build: clang++ -O3 -DNDEBUG -std=c++17 VoiceBench.cpp -o VoiceBench
// Run:   ./VoiceBench > bench_results_MACHINENAME.tsv
//
// Synthetic CPU budget benchmark for LAYERZ voice + grain caps.
// Run on each target machine; compare results to set PROFILE_PLUGIN and PROFILE_STANDALONE caps.
// Output: TSV with columns: buffer_size, n_voices, n_grains, peak_cpu, mean_cpu, stddev_cpu, overruns
//
// CPU% is relative to the real-time budget for one audio block at 44100 Hz sample rate.
// 100% = block took exactly as long as it would take to play (overrun).
// Target: mean_cpu < 25% at buffer=256 for PROFILE_PLUGIN cap.
//         mean_cpu < 40% at buffer=1024 for PROFILE_STANDALONE cap.
//
// Runs kNumWindows independent 10-second windows per config for statistical validity.
// stddev_cpu > 5% indicates machine noise — re-run on a quieter machine.

#include <cmath>
#include <chrono>
#include <vector>
#include <cstdio>
#include <cstring>
#include <algorithm>

static constexpr double kSampleRate    = 44100.0;
static constexpr int    kNumWindows    = 5;      // independent statistical windows per config
static constexpr int    kWindowSeconds = 10;     // duration of each window

// Sine oscillator proxy for a mono-synth voice
static float synth_voice_sample(float phase_norm, float freq_hz) {
    return std::sin(2.0f * 3.14159265358979f * phase_norm * freq_hz);
}

// Hann-windowed grain sample (proxy for BLOOM/PULVERIZE grain)
static float grain_sample(float phase_norm, float freq_hz, int sample_idx, int grain_len) {
    float window = 0.5f * (1.0f - std::cos(2.0f * 3.14159265358979f * sample_idx / (grain_len - 1)));
    return std::sin(2.0f * 3.14159265358979f * phase_norm * freq_hz) * window;
}

struct WindowResult {
    float mean_cpu_pct;
    float peak_cpu_pct;
    int   overruns;
};

struct BenchResult {
    float peak_cpu;    // worst window mean
    float mean_cpu;    // mean of window means
    float stddev_cpu;  // stddev of window means
    int   overruns;    // total overruns across all windows
};

static WindowResult run_window(int n_voices, int n_grains, int buffer_size) {
    const double target_block_us = (static_cast<double>(buffer_size) / kSampleRate) * 1e6;
    const int    blocks           = static_cast<int>(kWindowSeconds * kSampleRate / buffer_size);

    // Pre-allocate audio buffer (no allocation in the hot loop)
    std::vector<float> buf(buffer_size, 0.0f);

    // Phase state per voice and grain (normalized 0..1)
    std::vector<float> voice_phases(n_voices, 0.0f);
    std::vector<float> grain_phases(n_grains, 0.0f);

    float sum_cpu = 0.0f;
    float peak_cpu = 0.0f;
    int   overruns = 0;

    for (int b = 0; b < blocks; ++b) {
        std::fill(buf.begin(), buf.end(), 0.0f);

        // Use steady_clock — high_resolution_clock is not monotonic on Windows
        // and can produce negative elapsed times, corrupting the mean.
        auto t0 = std::chrono::steady_clock::now();

        // Simulate N simultaneous sine-oscillator voices
        for (int v = 0; v < n_voices; ++v) {
            float freq = 110.0f + static_cast<float>(v) * 37.0f;  // spread across spectrum
            for (int s = 0; s < buffer_size; ++s) {
                buf[s] += synth_voice_sample(voice_phases[v], freq) * (1.0f / static_cast<float>(n_voices));
                voice_phases[v] += 1.0f / kSampleRate;
                if (voice_phases[v] >= 1.0f) voice_phases[v] -= 1.0f;
            }
        }

        // Simulate M active grains (windowed reads)
        for (int g = 0; g < n_grains; ++g) {
            float freq = 55.0f + static_cast<float>(g) * 13.0f;
            for (int s = 0; s < buffer_size; ++s) {
                buf[s] += grain_sample(grain_phases[g], freq, s, buffer_size)
                          * (1.0f / static_cast<float>(n_grains));
                grain_phases[g] += 1.0f / kSampleRate;
                if (grain_phases[g] >= 1.0f) grain_phases[g] -= 1.0f;
            }
        }

        auto t1 = std::chrono::steady_clock::now();
        double elapsed_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        float  cpu_pct    = static_cast<float>(elapsed_us / target_block_us * 100.0);

        sum_cpu  += cpu_pct;
        if (cpu_pct > peak_cpu) peak_cpu = cpu_pct;
        if (elapsed_us > target_block_us) ++overruns;
    }

    WindowResult r;
    r.mean_cpu_pct = sum_cpu / static_cast<float>(blocks);
    r.peak_cpu_pct = peak_cpu;
    r.overruns     = overruns;
    return r;
}

static BenchResult aggregate(int n_voices, int n_grains, int buffer_size) {
    std::vector<float> window_means;
    window_means.reserve(kNumWindows);
    int total_overruns = 0;

    for (int w = 0; w < kNumWindows; ++w) {
        WindowResult wr = run_window(n_voices, n_grains, buffer_size);
        window_means.push_back(wr.mean_cpu_pct);
        total_overruns += wr.overruns;
    }

    float grand_mean = 0.0f;
    for (float m : window_means) grand_mean += m;
    grand_mean /= static_cast<float>(kNumWindows);

    float variance = 0.0f;
    for (float m : window_means) variance += (m - grand_mean) * (m - grand_mean);
    variance /= static_cast<float>(kNumWindows);

    float peak = *std::max_element(window_means.begin(), window_means.end());

    BenchResult r;
    r.peak_cpu  = peak;
    r.mean_cpu  = grand_mean;
    r.stddev_cpu = std::sqrt(variance);
    r.overruns  = total_overruns;
    return r;
}

int main() {
    // Header
    std::printf("buffer_size\tn_voices\tn_grains\tpeak_cpu\tmean_cpu\tstddev_cpu\toverruns\n");
    std::fflush(stdout);

    for (int buf : {256, 512, 1024}) {
        for (int v = 4; v <= 64; v += 4) {
            for (int g = 8; g <= 128; g += 8) {
                BenchResult r = aggregate(v, g, buf);
                std::printf("%d\t%d\t%d\t%.2f\t%.2f\t%.2f\t%d\n",
                    buf, v, g, r.peak_cpu, r.mean_cpu, r.stddev_cpu, r.overruns);
                std::fflush(stdout);
            }
        }
    }
    return 0;
}
