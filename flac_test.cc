#include <FLAC/stream_encoder.h>
#include <FLAC/metadata.h>
#include <stdio.h>

int main() {
    // Print FLAC library version
    printf("FLAC Library Version: %s\n", FLAC__VERSION_STRING);
    printf("FLAC Vendor String: %s\n", FLAC__VENDOR_STRING);
    
    // Create a simple encoder to test the library
    FLAC__StreamEncoder *encoder = FLAC__stream_encoder_new();
    if (encoder) {
        printf("Successfully created FLAC encoder!\n");
        
        // Check some capabilities
        printf("Max LPC order: %u\n", FLAC__stream_encoder_get_max_lpc_order(encoder));
        printf("Sample rate supported (44100 Hz): %s\n", 
               FLAC__format_sample_rate_is_valid(44100) ? "Yes" : "No");
        printf("Sample rate supported (48000 Hz): %s\n",
               FLAC__format_sample_rate_is_valid(48000) ? "Yes" : "No");
        
        FLAC__stream_encoder_delete(encoder);
        printf("FLAC library is working correctly!\n");
        return 0;
    } else {
        printf("Failed to create FLAC encoder\n");
        return 1;
    }
}
