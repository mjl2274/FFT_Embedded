#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h> // Include for memset

#define N 4096 // Number of points in FFT
#define SAMPLE_RATE 48000 // Sampling rate in Hz
#define FILENAME "HCB.wav" // Replace "audio.wav" with your audio file
#define FRACTIONAL_BITS 14 // Number of fractional bits for fixed-point representation


// Fixed-point data type
typedef int32_t fixed_point_t;

// Function to convert a double to fixed-point representation
fixed_point_t double_to_fixed(double value) {
    return (fixed_point_t)(value * (1 << FRACTIONAL_BITS));
}

// Function to convert a fixed-point value to double
double fixed_to_double(fixed_point_t value) {
    return (double)value / (1 << FRACTIONAL_BITS);
}

// Function to perform FFT
void fft(complex double x[], int n, int step) {
    if (n <= 1) return;

    // Divide
    int m = n / 2;
    complex double even[m], odd[m];
    for (int i = 0; i < m; i++) {
        even[i] = x[i * 2];
        odd[i] = x[i * 2 + 1];
    }

    // Conquer
    fft(even, m, step * 2);
    fft(odd, m, step * 2);

    // Combine
    for (int i = 0; i < m; i++) {
        complex double t = cexp(-I * 2 * M_PI * i / n * step) * odd[i];
        x[i] = even[i] + t;
        x[i + m] = even[i] - t;
    }
}

void ifft(complex double x[], int n, int step) {
    // Conjugate the input data
    for (int i = 0; i < n; i++) {
        x[i] = conj(x[i]);
    }

    // Apply forward FFT
    fft(x, n, step);

    // Normalize by the number of points
    for (int i = 0; i < n; i++) {
        x[i] /= n;
    }
}



// Function to read audio file and extract samples
bool read_audio(const char *filename, fixed_point_t *samples, int *num_samples) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Unable to open file.\n");
        return false;
    }

    // Skip WAV header
    fseek(file, 44, SEEK_SET);

    // Read audio samples (16-bit signed integers)
    int16_t buffer[N];
    *num_samples = fread(buffer, sizeof(int16_t), N, file);
    fclose(file);

    // Convert samples to fixed-point representation
    for (int i = 0; i < *num_samples; i++) {
        samples[i] = (fixed_point_t)buffer[i] << FRACTIONAL_BITS;
    }

    // Zero out the remaining elements in the samples array
    memset(samples + *num_samples, 0, (N - *num_samples) * sizeof(fixed_point_t));

    return true;
}

// Function to write time-domain samples to a WAV file
void write_wav_file(const char *filename, int16_t *samples, int num_samples, int sample_rate) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Error: Unable to open file for writing.\n");
        return;
    }

    // Write WAV header
    int16_t num_channels = 1; // Mono
    int16_t bits_per_sample = 16;
    int32_t data_size = num_samples * sizeof(int16_t);

    // WAV header
    fprintf(file, "RIFF"); // Chunk ID
    fwrite(&data_size, sizeof(int32_t), 1, file); // Chunk size
    fprintf(file, "WAVE"); // Format
    fprintf(file, "fmt "); // Subchunk1 ID
    int32_t subchunk1_size = 16;
    fwrite(&subchunk1_size, sizeof(int32_t), 1, file); // Subchunk1 size
    int16_t audio_format = 1; // PCM
    fwrite(&audio_format, sizeof(int16_t), 1, file); // Audio format
    fwrite(&num_channels, sizeof(int16_t), 1, file); // Num channels
    fwrite(&sample_rate, sizeof(int32_t), 1, file); // Sample rate
    int32_t byte_rate = sample_rate * num_channels * bits_per_sample / 8;
    fwrite(&byte_rate, sizeof(int32_t), 1, file); // Byte rate
    int16_t block_align = num_channels * bits_per_sample / 8;
    fwrite(&block_align, sizeof(int16_t), 1, file); // Block align
    fwrite(&bits_per_sample, sizeof(int16_t), 1, file); // Bits per sample
    fprintf(file, "data"); // Subchunk2 ID
    fwrite(&data_size, sizeof(int32_t), 1, file); // Subchunk2 size

    // Write audio samples
    fwrite(samples, sizeof(int16_t), num_samples, file);

    fclose(file);
}

int main() {
    // Example usage
    complex double x[N]; // Input sequence
    fixed_point_t samples[N]; // Array to store audio samples
    int16_t time_domain_samples[N];
    int num_samples;

    // Read audio file
    if (!read_audio(FILENAME, samples, &num_samples)) {
        return 1;
    }
    

    // Convert samples to complex array
    for (int i = 0; i < N; i++) {
        x[i] = fixed_to_double(samples[i]);
    }
    

    // Perform FFT
    fft(x, N, 1);

    // Print results
    printf("FFT result:\n");
    //for (int i = 0; i < N; i++) {
    //    printf("%.2f + %.2fi\n", creal(x[i]), cimag(x[i]));
    //}
    
    ifft(x, N, 1); // Assuming x contains the frequency-domain data

    // Convert the samples to integers if necessary
    for (int i = 0; i < N*SAMPLE_RATE; i++) {
        time_domain_samples[i] = (int16_t)round(creal(x[i])); // Assuming x is already in integer format
    }
    //printf("%.2f", time_domain_samples);
    // Write the time-domain samples to a WAV file
    write_wav_file("output.wav", time_domain_samples, N*SAMPLE_RATE, SAMPLE_RATE);

    return 0;
}
