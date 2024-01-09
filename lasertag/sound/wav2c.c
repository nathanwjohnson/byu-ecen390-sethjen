#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


// Leave the following line uncommented unless you want to generate a simple tone.
//#define GENERATE_TONE

#define MAX_FILENAME_LENGTH 512 // Max size for buffers.
#define WAV_SUFFIX "wav"        // The file must end in .wav
#define H_FILE_SUFFIX ".h"      // .h files have this suffix.
#define C_FILE_SUFFIX ".c"      // .c files have this suffix.
#define EXTERN_STATEMENT "extern"  // Just the C extern statement.
#define C_DATA_TYPE "uint16_t"  // Type for data in the .c file
#define SUPPORTED_WAVE_DATA_BIT_SIZE 16  // Program can only handle this size of data for now.

// Header-specific defines. All sizes are numbered in bytes.
#define CHUNKID "RIFF"          // String
#define CHUNKID_SIZE 5          // Make room for the NULL.
#define CHUNKSIZE_SIZE 4        // Number
#define FORMAT "WAVE"           // File must be of this FORMAT.
#define FORMAT_SIZE 5           // Make room for the NULL.
#define SUBCHUNK1ID "fmt "      // String.
#define SUBCHUNK1ID_SIZE 5      // Make room for NULL.
#define AUDIO_FORMAT_SIZE 2     // Number.
#define NUM_CHANNELS_SIZE 2     // Number.
#define SAMPLE_RATE_SIZE 4      // Number.
#define BYTE_RATE_SIZE 4        // Number.
#define BLOCK_ALIGN_SIZE 2      // Number.
#define BITS_PER_SAMPLE_SIZE 2  // Number.
#define SUBCHUNK2ID "data"      // String
#define SUBCHUNK2ID_SIZE 5      // Make room for NULL>
#define SUBCHUNK2SIZE 4         // Number.

// This is the header for the wave vile.
typedef struct  {
  char chunkId[CHUNKID_SIZE];     // Should contain "RIFF". Add room for NULL.
  uint32_t chunkSize;             // Size of the entire file minus 8 bytes (chunkSize + chunkSize).
  char format[FORMAT_SIZE];       // Should contain "WAVE". Add room for NULL.
  char subchunk1Id[FORMAT_SIZE];  // Should contain the letters "fmt ". Add room for NULL.
  uint32_t subchunk1Size;  // Should be 16 for PCM.
  uint16_t audioFormat;    // Will be 1 for PCM. This program can only handle PCM.
  uint16_t numChannels;    // Will be 1 for mono, 2 for stereo.
  uint32_t sampleRate;     // The sample rate.
  uint32_t byteRate;       // == sampleRate * numChannels * bitsPerSample/8.
  uint16_t blockAlign;     // == numChannels * bitsPerSample/8.
  uint16_t bitsPerSample;  // 8 bits, 16 bits, etc.
  char subchunk2Id[SUBCHUNK2ID_SIZE];     // Should contain "data". Add room for NULL.
  uint32_t subchunk2Size;  // Number of remaining bytes in the file (the actual data).
} waveFileHeader_t;

// Reads in the header.
void readWaveFileHeader(FILE* wavFile, waveFileHeader_t* header) {
  fread(header->chunkId, sizeof(header->chunkId)-1, 1, wavFile);
  header->chunkId[4] = '\0';  // Makes easier to print as a string.
  fread(&(header->chunkSize), 4, 1, wavFile);
  fread(header->format, sizeof(header->format)-1, 1, wavFile);
  header->format[4] = '\0';  // Makes easier to print as a string.
  fread(header->subchunk1Id, sizeof(header->subchunk1Id)-1, 1, wavFile);
  header->subchunk1Id[4] = '\0';  // Makes easier to print as a string.
  fread(&header->subchunk1Size, 4, 1, wavFile);
  fread(&header->audioFormat, 2, 1, wavFile);
  fread(&header->numChannels, 2, 1, wavFile);
  fread(&header->sampleRate, 4, 1, wavFile);
  fread(&header->byteRate, 4, 1, wavFile);
  fread(&header->blockAlign, 2, 1, wavFile);
  fread(&header->bitsPerSample, 2, 1, wavFile);
  fread(header->subchunk2Id, 4, 1, wavFile);
  header->subchunk2Id[4] = '\0';  // Makes easier to print as a string.
  fread(&header->subchunk2Size, 4, 1, wavFile);
}

// Prints the header to the outputStream.
void printWaveFileHeader(FILE* outputStream, waveFileHeader_t* header) {
  fprintf(outputStream, "chunkId:            \"%s\"\n", header->chunkId);
  fprintf(outputStream, "chunkSize:          %d\n", header->chunkSize);
  fprintf(outputStream, "format:             \"%s\"\n", header->format);
  fprintf(outputStream, "subchunkId:         \"%s\"\n", header->subchunk1Id);
  fprintf(outputStream, "subchunk1Size:      %d\n", header->subchunk1Size);
  fprintf(outputStream, "audio format:       %d\n", header->audioFormat);
  fprintf(outputStream, "number of channels: %d\n", header->numChannels);
  fprintf(outputStream, "sample rate:        %d\n", header->sampleRate);
  fprintf(outputStream, "byte rate:          %d\n", header->byteRate);
  fprintf(outputStream, "block align:        %d\n", header->blockAlign);
  fprintf(outputStream, "bits per sample:    %d\n", header->bitsPerSample);
  fprintf(outputStream, "subchunk2Id:        \"%s\"\n", header->subchunk2Id);
  fprintf(outputStream, "subchunk2Size:      %d\n", header->subchunk2Size);
}

// Returns true if the header looks OK by inspecting various fixed-value fields, false otherwise.
// Prints out informational messages if the os (output stream) argument is non-NULL.
bool waveFileHeaderOk(FILE* os, waveFileHeader_t* header) {
  bool isOk = true;  // Be optimistic.
  if (strncmp(header->chunkId, CHUNKID, CHUNKID_SIZE)) {
    isOk = false;
    if (os)
      fprintf(os, "ERROR: chunkId (%s) is incorrect. Should be: %s\n", header->chunkId, CHUNKID); 
  }
  if (strncmp(header->format, FORMAT, FORMAT_SIZE)) {
    isOk = false;
    if (os)
      fprintf(os, "ERROR: format (%s) is incorrect. Should be: %s\n", header->format, FORMAT); 
  }
  if (strncmp(header->subchunk1Id, SUBCHUNK1ID, SUBCHUNK1ID_SIZE)) {
    isOk = false;
    if (os)
      fprintf(os, "ERROR: subchunkId (%s) is incorrect. Should be: %s\n", header->subchunk1Id, SUBCHUNK1ID); 
  }
  if (strncmp(header->subchunk2Id, SUBCHUNK2ID, SUBCHUNK2ID_SIZE)) {
    isOk = false;
    if (os)
      fprintf(os, "ERROR: subchunk2Id (%s) is incorrect. Should be: %s\n", header->subchunk2Id, SUBCHUNK2ID); 
  }
  return isOk;
}

// Return the file's extension (.suffix). 
const char *get_filename_extension(const char* fileName) {
  const char *dot = strrchr(fileName, '.');  // Find the last occurrence of "."
  if (!dot || dot == fileName) return "";    // If "." doesn't exist or if file name starts with ".", return empty string.
  return dot + 1;                            // Advance to the string that follows "."
}

int main(int argc, char* argv[]) {
  // Print a helpful error message and exit if a file-name was not provided on the command line.
  if (argc != 2) {
    fprintf(stderr, "Usage: wav2c filename.wav\n");
    exit(-1);
  }
  char inputFileName[MAX_FILENAME_LENGTH];         // Create a working buffer.
  strncpy(inputFileName, argv[1], MAX_FILENAME_LENGTH);  // Copy the argv[1] filename into the working buffer.
  // Make sure that the file-name has a .wav suffix.
  const char* extension = get_filename_extension(inputFileName);  // Get the suffix.
  // Compare the suffix and generate an error message if it is incorrect.
  if (strncmp(extension, WAV_SUFFIX, MAX_FILENAME_LENGTH)) {              
    fprintf(stderr, "ERROR: input file-name \"%s\" does not have a %s suffix.\n", inputFileName, WAV_SUFFIX);
    exit(-1);  // Exit because of the error.
  }
  
  // Try to open the file.
  FILE* inputFileFp = fopen(inputFileName, "rb");
  if (inputFileFp == NULL) {
    printf("unable to find file:%s\n", argv[1]);
    exit(-1);
  }
  // File is present and has correct extension if you get this far.
  waveFileHeader_t header;
  readWaveFileHeader(inputFileFp, &header);  // Read the header.
  if (!waveFileHeaderOk(stderr, &header)) {  // Check the header.
    fprintf(stderr, "ERROR: Input file (%s) contains errors. Maybe not a .wav file? See proceeding messages for details.\n", inputFileName);
    printWaveFileHeader(stderr, &header);
    exit(-1);
  }
  // File contents look good if you get this far.
  // Only handle mono files for now, check for this and exit if the file is not mono.
  if (header.numChannels > 1) {
    fprintf(stderr, "INFO: wave file contains stereo data. This program only handles monophonic data for now. Exiting...\n");
    exit(-1);
  }
  // Only handle 16-bit data for now.
  if (header.bitsPerSample != SUPPORTED_WAVE_DATA_BIT_SIZE) {
    fprintf(stderr, "INFO: wave file contains %d-bit data. This program can only process %d-bit data. Exiting...\n", header.bitsPerSample, SUPPORTED_WAVE_DATA_BIT_SIZE);
    exit(-1);
  }
  printWaveFileHeader(stderr, &header);
  // Everything looks good. Go ahead and generate the .h and .c files. Exit with an error if either file already exists.
  // First, open the .h file and output the necessary declarations. Then, close the .h file.
  char hFileName[MAX_FILENAME_LENGTH];                     // .h file-name.
  strncpy(hFileName, inputFileName, MAX_FILENAME_LENGTH);  // Basename is just the input-file-name.
  strncat(hFileName, H_FILE_SUFFIX, MAX_FILENAME_LENGTH);  // Add the suffix.
  char cFileName[MAX_FILENAME_LENGTH];                     // .c file-name. 
  strncpy(cFileName, inputFileName, MAX_FILENAME_LENGTH);  // Basename is just the input-file-name.
  strncat(cFileName, C_FILE_SUFFIX, MAX_FILENAME_LENGTH);  // Add the suffix.

  FILE* hFileFp = fopen(hFileName, "r");
  FILE* cFileFp = fopen(cFileName, "r");
  // If you get this far, you can go ahead and open the .c and .h files for write.
  hFileFp = fopen(hFileName, "w");
  cFileFp = fopen(cFileName, "w");
  // Ensure that both files opened OK for write.
  if ((!hFileFp && cFileFp)) {
    if (!hFileFp)
      fprintf(stderr, "Unable to open file: %s for writing.\n", hFileName);
    if (!cFileFp)
      fprintf(stderr, "Unable to open file: %s for writing.\n", cFileName);
    exit(-1);
  }
  char arrayName[MAX_FILENAME_LENGTH];  // Array name will be created here.
  uint32_t i = 0;
  // Need to create an array name with no ".". Replace all "." with underscore.
  for (i=0; inputFileName[i] != '\0'; i++) {
    if (inputFileName[i] == '.') {
      arrayName[i] = '_';
    } else {
      arrayName[i] = inputFileName[i];
    }
    arrayName[i+1] = '\0';  // Make sure to terminate the string.
  }
  // Generate an upper-case version of arrayName;
  char arrayNameUpperCase[MAX_FILENAME_LENGTH];
  for (i=0; arrayName[i] != '\0'; i++) {
    arrayNameUpperCase[i] = toupper(arrayName[i]);
  }
  arrayNameUpperCase[i+1] = '\0';  // Make sure to terminate the string.
 
  // .h file just needs a comment and an extern statement.
  fprintf(hFileFp, "// This file was generated by executing this statement: wav2c %s\n", inputFileName);
  fprintf(hFileFp, "%s uint16_t %s[];\n", EXTERN_STATEMENT, arrayName);
  fprintf(hFileFp, "#define %s_SAMPLE_RATE %d\n", arrayNameUpperCase, header.sampleRate*10);
  fprintf(hFileFp, "#define %s_BITS_PER_SAMPLE %d\n", arrayNameUpperCase, header.bitsPerSample);
  fprintf(hFileFp, "#define %s_NUMBER_OF_SAMPLES %d\n", arrayNameUpperCase, header.subchunk2Size/2);
  fclose(hFileFp);
  // .c file will contain the data in array form as follows.
  uint32_t arraySize = header.subchunk2Size/2;  // Wave file is counted by bytes, output file is counted in 16-bit words.
  // Write some helpful comments to the .c file.
  fprintf(cFileFp, "// This file was generated by executing this statement: wav2c %s\n", inputFileName);
  fprintf(cFileFp, "\n#include <stdint.h>\n\n");
  fprintf(cFileFp, "%s %s[%d] = {\n", C_DATA_TYPE, arrayName, arraySize);
  // File pointer to the input file should be pointing at the first value after the header is read, so just start from there.
  for (i=0; i<header.subchunk2Size/2; i++) {
    int16_t data;  // Program only handles 16-bit PCM data for now. PCM data is signed.
    fread(&data, header.blockAlign, 1, inputFileFp);  // Read 2 bytes into a signed 16-bit variable.
    uint16_t unsignedData = data + INT16_MAX;         // Offset to unsigned for the sound CODEC.
    fprintf(cFileFp, "%d", unsignedData);             // Write the unsiged data.
    if (i != (header.subchunk2Size/2)-1)              // Don't place the last comma.
      fprintf(cFileFp, ",\n");                        // Delimited data.
  }
  fprintf(cFileFp, "\n};\n");  // Close the array.
  fclose(cFileFp);             // Close the .h file.
  fclose(inputFileFp);         // Close the input file.
}
