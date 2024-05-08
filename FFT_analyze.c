#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h> // Include for memset

#define N 8192 // Number of points in FFT
#define SAMPLE_RATE 48000 // Sampling rate in Hz
#define FILENAME "HCB.wav" // Replace "audio.wav" with your audio file
#define FRACTIONAL_BITS 14 // Number of fractional bits for fixed-point representation
#define NUM_TOP_FREQUENCIES 5

// Define the fixed-point data type
typedef int32_t fixed_point_t;

// Function to convert a double to fixed-point representation
fixed_point_t double_to_fixed(double value) {
    return (fixed_point_t)(value * (1 << FRACTIONAL_BITS));
}

// Function to convert a fixed-point value to double
double fixed_to_double(fixed_point_t value) {
    return (double)value / (1 << FRACTIONAL_BITS);
}

// Function to perform FFT and compute magnitude spectrum
void fft_and_magnitude(complex double x[], fixed_point_t magnitude_spectrum[], int n, int step) {
    if (n <= 1) return;

    // Divide
    int m = n / 2;
    complex double even[m], odd[m];
    for (int i = 0; i < m; i++) {
        even[i] = x[i * 2];
        odd[i] = x[i * 2 + 1];
    }

    // Conquer
    fft_and_magnitude(even, magnitude_spectrum, m, step * 2);
    fft_and_magnitude(odd, magnitude_spectrum, m, step * 2);

    // Combine
    for (int i = 0; i < m; i++) {
        complex double t = cexp(-I * 2 * M_PI * i / n * step) * odd[i];
        x[i] = even[i] + t;
        x[i + m] = even[i] - t;
    }

    // Compute magnitude spectrum
    for (int i = 0; i < n / 2; i++) {
        magnitude_spectrum[i] = cabs(x[i]);
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
//Bandpass Filter
void apply_bandpass_filter(fixed_point_t magnitude_spectrum[], int lower_bin, int upper_bin) {
    for (int i = 0; i < N / 2; i++) {
        if (i < lower_bin || i > upper_bin) {
            magnitude_spectrum[i] = 0; // Zero out frequencies outside the bandpass range
        }
    }
}
//Moving Average across FFT to smooth it and make it easier to detect peaks
void apply_moving_average_filter(fixed_point_t magnitude_spectrum[], int window_size) {
    fixed_point_t smoothed_spectrum[N / 2];
    for (int i = 0; i < N / 2; i++) {
        int start_index = fmax(0, i - window_size / 2);
        int end_index = fmin(N / 2 - 1, i + window_size / 2);
        fixed_point_t sum = 0;
        for (int j = start_index; j <= end_index; j++) {
            sum += magnitude_spectrum[j];
        }
        smoothed_spectrum[i] = sum / (end_index - start_index + 1);
    }
    memcpy(magnitude_spectrum, smoothed_spectrum, sizeof(smoothed_spectrum));
}

// Function to find the index of the peak in the magnitude spectrum
int find_peak_index(fixed_point_t *magnitude_spectrum, int num_samples) {
    int peak_index = 0;
    fixed_point_t max_magnitude = magnitude_spectrum[0];
    for (int i = 1; i < num_samples / 2; i++) {
        if (magnitude_spectrum[i] > max_magnitude) {
            max_magnitude = magnitude_spectrum[i];
            peak_index = i;
        }
    }
    return peak_index;
}

// Function to find the nearest piano note frequency to a given frequency
// Function to find the nearest piano note frequency to a given frequency
double find_nearest_note_frequency(double frequency) {
    // Array of piano note frequencies
    const double note_frequencies[] = {
        27.50, 29.14, 30.87, 32.70, 34.65, 36.71, 38.89, 41.20, 43.65, 46.25, 49.00, 51.91, 55.00, 58.27, 61.74, 65.41,
        69.30, 73.42, 77.78, 82.41, 87.31, 92.50, 98.00, 103.83, 110.00, 116.54, 123.47, 130.81, 138.59, 146.83, 155.56,
        164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94, 261.63, 277.18, 293.66, 311.13, 329.63, 349.23,
        369.99, 392.00, 415.30, 440.00, 466.16, 493.88, 523.25, 554.37, 587.33, 622.25, 659.26, 698.46, 739.99, 783.99,
        830.61, 880.00, 932.33, 987.77, 1046.50, 1108.73, 1174.66, 1244.51, 1318.51, 1396.91, 1479.98, 1567.98, 1661.22,
        1760.00, 1864.66, 1975.53, 2093.00, 2217.46, 2349.32, 2489.02, 2637.02, 2793.83, 2959.96, 3135.96, 3322.44, 3520.00,
        3729.31, 3951.07, 4186.01
    };
    // here middle C is 261.63, middle D is 261.63, 

    // Find the nearest note frequency
    double nearest_frequency = note_frequencies[0];
    double min_difference = fabs(frequency - nearest_frequency);
    for (int i = 1; i < sizeof(note_frequencies) / sizeof(note_frequencies[0]); i++) {
        double difference = fabs(frequency - note_frequencies[i]);
        if (difference < min_difference) {
            nearest_frequency = note_frequencies[i];
            min_difference = difference;
        }
    }
    return nearest_frequency;
}


#include <stdio.h>

// Function to map frequency to piano note
const char *map_frequency_to_note(double frequency) {
    // Array of piano note frequencies
    const double note_frequencies[] = {
        /* Frequency values of piano notes */
        27.50, 29.14, 30.87, 32.70, 34.65, 36.71, 38.89, 41.20, 43.65, 46.25, 49.00, 51.91, 
        55.00, 58.27, 61.74, 65.41, 69.30, 73.42, 77.78, 82.41, 87.31, 92.50, 98.00, 103.83, 
        110.00, 116.54, 123.47, 130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 
        207.65, 220.00, 233.08, 246.94, 261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 
        392.00, 415.30, 440.00, 466.16, 493.88, 523.25, 554.37, 587.33, 622.25, 659.26, 698.46, 
        739.99, 783.99, 830.61, 880.00, 932.33, 987.77, 1046.50, 1108.73, 1174.66, 1244.51, 1318.51, 
        1396.91, 1479.98, 1567.98, 1661.22, 1760.00, 1864.66, 1975.53, 2093.00, 2217.46, 2349.32, 
        2489.02, 2637.02, 2793.83, 2959.96, 3135.96, 3322.44, 3520.00, 3729.31, 3951.07, 4186.01
    };

    // Array of piano note names
    const char* note_names[] = {
        /* Names of piano notes */
        "A0", "A#0/Bb0", "B0", "C1", "C#1/Db1", "D1", "D#1/Eb1", "E1", "F1", "F#1/Gb1", "G1", "G#1/Ab1",
        "A1", "A#1/Bb1", "B1", "C2", "C#2/Db2", "D2", "D#2/Eb2", "E2", "F2", "F#2/Gb2", "G2", "G#2/Ab2",
        "A2", "A#2/Bb2", "B2", "C3", "C#3/Db3", "D3", "D#3/Eb3", "E3", "F3", "F#3/Gb3", "G3", "G#3/Ab3",
        "A3", "A#3/Bb3", "B3", "C4", "C#4/Db4", "D4", "D#4/Eb4", "E4", "F4", "F#4/Gb4", "G4", "G#4/Ab4",
        "A4", "A#4/Bb4", "B4", "C5", "C#5/Db5", "D5", "D#5/Eb5", "E5", "F5", "F#5/Gb5", "G5", "G#5/Ab5",
        "A5", "A#5/Bb5", "B5", "C6", "C#6/Db6", "D6", "D#6/Eb6", "E6", "F6", "F#6/Gb6", "G6", "G#6/Ab6",
        "A6", "A#6/Bb6", "B6", "C7", "C#7/Db7", "D7", "D#7/Eb7", "E7", "F7", "F#7/Gb7", "G7", "G#7/Ab7",
        "A7", "A#7/Bb7", "B7", "C8"
    };

    // Find the nearest note frequency
    double nearest_frequency = note_frequencies[0];
    double min_difference = fabs(frequency - nearest_frequency);
    for (int i = 1; i < sizeof(note_frequencies) / sizeof(note_frequencies[0]); i++) {
        double difference = fabs(frequency - note_frequencies[i]);
        if (difference < min_difference) {
            nearest_frequency = note_frequencies[i];
            min_difference = difference;
        }
    }

    // Find the index of the nearest frequency in the note frequencies array
    int index = 0;
    for (int i = 0; i < sizeof(note_frequencies) / sizeof(note_frequencies[0]); i++) {
        if (note_frequencies[i] == nearest_frequency) {
            index = i;
            break;
        }
    }

    // Return the corresponding note name
    return note_names[index];
}

void find_top_n_peaks(fixed_point_t *magnitude_spectrum, int num_samples, int top_n_indices[], int n) {
    int peaks_found = 0;
    bool is_peak = true;
    for (int i = 1; i < num_samples / 2 - 1; i++) {
        if (magnitude_spectrum[i] > magnitude_spectrum[i - 1] && magnitude_spectrum[i] > magnitude_spectrum[i + 1]) {
            is_peak = true;
            for (int j = 0; j < n; j++) {
                if (i == top_n_indices[j]) {
                    is_peak = false; // Ensure no duplicate peaks
                    break;
                }
            }
            if (is_peak) {
                top_n_indices[peaks_found] = i;
                peaks_found++;
                if (peaks_found >= n) break;
            }
        }
    }
}



int main() {
    // Example usage
    complex double x[N]; // Input sequence
    fixed_point_t samples[N]; // Array to store audio samples
    fixed_point_t magnitude_spectrum[N / 2]; // Array to store magnitude spectrum
    int num_samples;

    // Read audio file
    if (!read_audio(FILENAME, samples, &num_samples)) {
        return 1;
    }

    // Convert samples to complex array
    for (int i = 0; i < N; i++) {
        x[i] = fixed_to_double(samples[i]);
    }

    // Perform FFT and compute magnitude spectrum
    fft_and_magnitude(x, magnitude_spectrum, N, 1);
    
	//Convert to double
    for (int i = 0; i < N / 2; i++) {
        magnitude_spectrum[i] = fixed_to_double(magnitude_spectrum[i]);
    }
    //Smoothing
    //apply_moving_average_filter(magnitude_spectrum, 3);
    
    // Apply bandpass filter to the magnitude spectrum
    int lower_frequency = 27; // Lower cutoff frequency in Hz
    int upper_frequency = 4200; // Upper cutoff frequency in Hz
    int lower_bin = lower_frequency * N / SAMPLE_RATE;
    int upper_bin = upper_frequency * N / SAMPLE_RATE;
    apply_bandpass_filter(magnitude_spectrum, lower_bin, upper_bin);
    
    // Calculate mean magnitude up to 4200 Hz
    double mean_magnitude = 0.0;
    int upper_frequency_bin = 4200 * N / SAMPLE_RATE; // Frequency bin corresponding to 4200 Hz
    for (int i = 0; i < upper_frequency_bin && i < N / 2; i++) {
        mean_magnitude += fixed_to_double(magnitude_spectrum[i]);
    }
    mean_magnitude /= upper_frequency_bin;

    // Print mean magnitude
    printf("Mean Magnitude up to 4200 Hz: %.2f\n", mean_magnitude);

    // Calculate standard deviation
    double standard_deviation = 0.0;
    for (int i = 0; i < N / 2; i++) {
        if (i < upper_frequency_bin) {
            double deviation = fixed_to_double(magnitude_spectrum[i]) - mean_magnitude;
            standard_deviation += deviation * deviation;
        }
    }
    standard_deviation = sqrt(standard_deviation / upper_frequency_bin);

    // Print standard deviation
    printf("Standard Deviation up to 4200 Hz: %.2f\n", standard_deviation);

    // Print frequencies above the mean plus 1 standard deviation
    printf("Frequencies above the mean plus 1 standard deviation:\n");
    for (int i = 0; i < N / 2; i++) {
        double magnitude = fixed_to_double(magnitude_spectrum[i]);
        if (i < upper_frequency_bin && magnitude > mean_magnitude + (1*standard_deviation)) {
            double frequency = (double)i * SAMPLE_RATE / N;
            printf("%.2f Hz\n", frequency);
        }
    }


	//1 frequency!

    // Find the peak index in the magnitude spectrum
    //int peak_index = find_peak_index(magnitude_spectrum, N);

    // Calculate the frequency corresponding to the peak index
    //double peak_frequency = (double)peak_index * SAMPLE_RATE / N;

    // Find the nearest piano note frequency to the peak frequency
    //double nearest_note_frequency = find_nearest_note_frequency(peak_frequency);

    // Print results
    //printf("Peak frequency: %.2f Hz\n", peak_frequency);
    //printf("Nearest piano note frequency: %.2f Hz\n", nearest_note_frequency);
    
    //a couple frequencies
    
    // Find the indices of the top N peaks in the magnitude spectrum
    int top_n_indices[NUM_TOP_FREQUENCIES] = {0};
    find_top_n_peaks(magnitude_spectrum, N / 2, top_n_indices, NUM_TOP_FREQUENCIES);

    // Print the top N frequencies and their corresponding notes
    for (int i = 0; i < NUM_TOP_FREQUENCIES; i++) {
        double frequency = (double)top_n_indices[i] * SAMPLE_RATE / N;
        const char *note = map_frequency_to_note(frequency);
        printf("Peak frequency %d: %.2f Hz, Nearest piano note frequency: %.2f Hz\n", i + 1, frequency, note);
    }

    return 0;
}
