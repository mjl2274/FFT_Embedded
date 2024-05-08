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

// Function to read audio file and extract samples
bool read_audio(const char *filename, double *samples, int *num_samples) {
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

    // Convert samples to doubles in the range [-1, 1]
    for (int i = 0; i < *num_samples; i++) {
        samples[i] = (double)buffer[i] / 32768.0;
    }

    // Zero out the remaining elements in the samples array
    memset(samples + *num_samples, 0, (N - *num_samples) * sizeof(double));

    return true;
}

int main() {
    // Example usage
    complex double x[N]; // Input sequence
    double samples[N]; // Array to store audio samples
    int num_samples;

    // Read audio file
    if (!read_audio(FILENAME, samples, &num_samples)) {
        return 1;
    }

    // Copy samples to complex array
    for (int i = 0; i < N; i++) {
        x[i] = samples[i];
    }

    // Perform FFT
    fft(x, N, 1);

    // Print results
    printf("FFT result:\n");
    for (int i = 0; i < N; i++) {
        printf("%.2f + %.2fi\n", creal(x[i]), cimag(x[i]));
    }

    return 0;
}
