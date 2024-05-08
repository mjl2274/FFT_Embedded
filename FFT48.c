#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Define constants
#define SAMPLE_RATE 48000
#define CHUNK_SIZE 4096
#define MIN_SEGMENT_DURATION_SEC 2
#define MAX_SEGMENT_DURATION_SEC 44
#define FILENAME "HCB.wav"
#define FRACTIONAL_BITS 14

#ifndef PI
# define PI	3.14159265358979323846264338327950288
#endif

// Define types
typedef int32_t fixed_point_t;
typedef complex double fft_complex_t;

// Function declarations
bool read_audio(const char *filename, fixed_point_t *samples, int *num_samples);
void apply_fft(fixed_point_t *samples, fft_complex_t *fft_output, int num_samples);
void analyze_frequency_spectrum(fft_complex_t *fft_output, int num_samples);
const char *map_frequency_to_note(double frequency);
void apply_bandpass_filter(fft_complex_t magnitude_spectrum[], int lower_bin, int upper_bin);
void process_audio(const char *filename);

int main() {
    process_audio(FILENAME);
    return 0;
}


/*
void print_first_and_last(fft_complex_t array[], int length) {
	double complex z;
    printf("First 5 elements: ");
    for (int i = 0; i < 5 && i < length; i++) {
    	z = array[i];
        printf("%.2f +.2fi ", creal(z), cimag(z));
    }
    printf("\n");

    printf("Last 5 elements: ");
    for (int i = length - 5; i < length && i >= 0; i++) {
    	z = array[i];
        printf("%.2f + %.2fi ", creal(z), cimag(z));
    }
    printf("\n");
}
*/

void print_first_and_last(double array[], int length) {
    printf("First 5 elements: ");
    for (int i = 0; i < 5 && i < length; i++) {
        printf("%f ", array[i]);
    }
    printf("\n");

    printf("Last 5 elements: ");
    for (int i = length - 5; i < length && i >= 0; i++) {
        printf("%f ", array[i]);
    }
    printf("\n");
}

void normalize_fft_output(fft_complex_t *fft_output, int num_samples) {
    // Find the maximum magnitude in the FFT output
    double max_magnitude = 0.0;
    for (int i = 0; i < num_samples; i++) {
        double magnitude = cabs(fft_output[i]);
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
        }
    }

    // Normalize the FFT output
    if (max_magnitude > 0.0) {
        for (int i = 0; i < num_samples; i++) {
            fft_output[i] /= max_magnitude;
        }
    }
}



//magnitude
void calculate_magnitude_spectrum(fft_complex_t *fft_output, int num_samples, double *magnitude_spectrum) {
    for (int i = 0; i < num_samples; i++) {
        magnitude_spectrum[i] = cabs(fft_output[i]);
    }
}

// Function to read audio file
bool read_audio(const char *filename, fixed_point_t *samples, int *num_samples) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Unable to open file.\n");
        return false;
    }

    // Skip WAV header
    fseek(file, 44, SEEK_SET);

    // Read audio samples (16-bit signed integers) 
    int16_t buffer[CHUNK_SIZE];
    *num_samples = fread(buffer, sizeof(int16_t), CHUNK_SIZE, file);
    fclose(file);

    // Convert samples to fixed-point representation
    for (int i = 0; i < *num_samples; i++) {
        samples[i] = (fixed_point_t)buffer[i] << FRACTIONAL_BITS;
    }

    // Zero out the remaining elements in the samples array
    memset(samples + *num_samples, 0, (CHUNK_SIZE - *num_samples) * sizeof(fixed_point_t));

    return true;
}


// Function to apply FFT to audio samples
void apply_fft(fixed_point_t *samples, fft_complex_t *fft_output, int num_samples) {
    if (num_samples <= 1) return;

    // Divide
    int m = num_samples / 2;
    fixed_point_t even[m], odd[m]; // Changed from fft_complex_t to fixed_point_t
    for (int i = 0; i < m; i++) {
        even[i] = samples[i * 2];
        odd[i] = samples[i * 2 + 1];
    }

    // Conquer
    apply_fft(even, fft_output, m);
    apply_fft(odd, fft_output + m, m);

    // Combine
    for (int i = 0; i < m; i++) {
        fft_complex_t t = cexp(-I * 2 * M_PI * i / num_samples) * odd[i];
        fft_output[i] = even[i] + t;
        fft_output[i + m] = even[i] - t;
    }
}


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

void analyze_frequency_spectrum(fft_complex_t *fft_output, int num_samples) {
    // Calculate the bin width and maximum frequency
    double bin_width = (double)SAMPLE_RATE / num_samples;
    double max_frequency = 4200;

    // Array to store the magnitude of each frequency bin
    double magnitudes[num_samples / 2];

    // Calculate magnitudes of frequency bins
    for (int i = 0; i < num_samples / 2; i++) {
        double magnitude = cabs(fft_output[i]);
        magnitudes[i] = magnitude;
    }

    // Find the top three frequencies
    double top_magnitudes[3] = {0};
    double top_frequencies[3] = {0};
    int top_indices[3] = {-1, -1, -1}; // Stores the indices of the top frequencies
    for (int i = 0; i < num_samples / 2; i++) {
        if (magnitudes[i] > top_magnitudes[0]) {
            top_magnitudes[2] = top_magnitudes[1];
            top_indices[2] = top_indices[1];
            top_magnitudes[1] = top_magnitudes[0];
            top_indices[1] = top_indices[0];
            top_magnitudes[0] = magnitudes[i];
            top_indices[0] = i;
        } else if (magnitudes[i] > top_magnitudes[1]) {
            top_magnitudes[2] = top_magnitudes[1];
            top_indices[2] = top_indices[1];
            top_magnitudes[1] = magnitudes[i];
            top_indices[1] = i;
        } else if (magnitudes[i] > top_magnitudes[2]) {
            top_magnitudes[2] = magnitudes[i];
            top_indices[2] = i;
        }
    }
    for(int i = 0; i < 3; i++) {
    	top_frequencies[i] = top_indices[i] * bin_width;
        }
    // Print the top three frequencies
    for (int i = 0; i < 3; i++) {
        printf("Top Frequency %d: %.2f Hz\n", i + 1, top_frequencies[i]);
    }

    // Map frequencies to notes and print
    for (int i = 0; i < 3; i++) {
        double frequency = top_frequencies[i];
        const char *note = map_frequency_to_note(frequency);
        printf("Mapped Note %d: %s\n", i + 1, note);
    }
}


//Bandpass Filter
void apply_bandpass_filter(fft_complex_t magnitude_spectrum[], int lower_bin, int upper_bin) {
    for (int i = 0; i < CHUNK_SIZE; i++) {
        if (i < lower_bin || i > upper_bin) {
            magnitude_spectrum[i] = 0; // Zero out frequencies outside the bandpass range
        }
    }
}

// Function to process audio file
void process_audio(const char *filename) {
    fixed_point_t samples[CHUNK_SIZE];
    fft_complex_t fft_output[CHUNK_SIZE];
    fft_complex_t fft_temp[CHUNK_SIZE];

    // Open the audio file
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Unable to open file.\n");
        return;
    }

    // Skip WAV header
    fseek(file, 44, SEEK_SET);

    // Process audio in chunks until the desired segment duration is reached
    double segment_duration1 = 0;
    double segment_duration = 0; 
    while (segment_duration < MAX_SEGMENT_DURATION_SEC) {
        // Read a chunk of audio samples
        size_t num_samples_read = fread(samples, sizeof(fixed_point_t), CHUNK_SIZE, file);

        // Check if the chunk contains enough samples
        if (num_samples_read < CHUNK_SIZE) {
            // Handle incomplete chunk (optional)
            break;
        }

        // Apply bandpass filter
        //apply_bandpass_filter(samples, 27, 4200);
        
        while(segment_duration1 < MIN_SEGMENT_DURATION_SEC){
        
        	// Apply FFT to the chunk
        	apply_fft(samples, fft_temp, CHUNK_SIZE);
        
        	for (int i = 0; i < CHUNK_SIZE; i++) {
    			fft_output[i] += fft_temp[i];
			}
        	// Update segment duration
        	segment_duration1 += (double)CHUNK_SIZE / SAMPLE_RATE;
        	segment_duration += (double)CHUNK_SIZE / SAMPLE_RATE;
        	//printf("%f", segment_duration);
        }
        segment_duration1 = 0;
        //printf("%f\n", segment_duration);
        
        //Normalize attempt 1:
        normalize_fft_output(fft_output, CHUNK_SIZE);
        
        apply_bandpass_filter(fft_output, 1, 360);
        
        
        //Magnitude Spectrum for nonsense
        //double magnitude_spectrum[CHUNK_SIZE];
        //calculate_magnitude_spectrum(fft_output, CHUNK_SIZE, magnitude_spectrum);
    	
    	//Structure for printing the fft_output array to check filter
    	/*
        printf("First 5 elements: ");
    	for (int i = 0; i < 5; i++) {
        	printf("%.2f + %.2fi ", creal(fft_output[i]), cimag(fft_output[i]));
    		}
    	printf("\n");

    	printf("Last 5 elements: ");
    	for (int i = 4091; i < 4097; i++) {
        	printf("%.2f + %.2fi ", creal(fft_output[i]), cimag(fft_output[i]));
    	}
   	 	printf("\n"); 
		*/
		//Structure to check the magnitude spectrum
    	//print_first_and_last(magnitude_spectrum, CHUNK_SIZE);
    	
        // Analyze frequency content of the chunk
        analyze_frequency_spectrum(fft_output, CHUNK_SIZE);
        
        //clear FFT array for processing the next chunk
        for (int i = 0; i < CHUNK_SIZE; i++) {
    			fft_output[i] = 0;
			}

        // Update segment duration
        //segment_duration += (double)CHUNK_SIZE / SAMPLE_RATE;
    }

    // Close the audio file
    fclose(file);
}
