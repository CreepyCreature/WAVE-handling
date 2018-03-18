#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <memory>

#define _USE_MATH_DEFINES
#include <cmath>  
#include <math.h>

struct HEADER_WAV
{
	char RIFF[4];
	uint32_t CHUNK_SIZE;
	char WAVE[4];
	char fmt[4];
	uint32_t SUBCHUNK1_SIZE;
	uint16_t AUDIO_FORMAT;
	uint16_t CHANNELS;
	uint32_t SAMPLES;
	uint32_t BYTES_PER_SEC;
	uint16_t BLOCK_ALIGN;
	uint16_t BITRATE;
	char data[4];
	uint32_t SUBCHUNK2_SIZE;
};

void PrintWAVHeader(HEADER_WAV fileheader);
void GenSineWAV(const char* filename, float seconds, float freq, float amplitude, int channels = 1, int sample_rate = 8000, int bitrate = 16);
std::unique_ptr<int16_t> GenSine(float seconds, float freq, float amplitude, int sample_rate = 8000);
void WriteWAV(const char* filename, HEADER_WAV header, const char* data);
HEADER_WAV GenWAVHeader(int channels, int num_samples, int sample_rate, int bitrate);

int main()
{
	std::string filestr;
	std::cout << "Please enter the path to a WAVE (.wav) file.." << std::endl;
	std::getline(std::cin, filestr);

	std::ifstream in(
		filestr.c_str(), 
		std::ios::in | std::ios::binary
	);
	if (!in.is_open())
	{
		std::cout << "Could not open file!" << std::endl;
		std::cin.get();
		return 1;
	}
	
	HEADER_WAV fileheader;
	in.read(reinterpret_cast<char*>(&fileheader), sizeof(fileheader));
	PrintWAVHeader(fileheader);

	std::ofstream out("output_samples");
	std::cout << "Writing sample values to file..." << std::endl;
	if (fileheader.BITRATE == 16)
	{
		int16_t* samples16 = new int16_t[std::ceil(fileheader.SUBCHUNK2_SIZE / (float)sizeof(int16_t))];
		in.read(reinterpret_cast<char*>(samples16), fileheader.SUBCHUNK2_SIZE);
		out.write(reinterpret_cast<char*>(samples16), fileheader.SUBCHUNK2_SIZE);

		delete[] samples16;
	}
	else if (fileheader.BITRATE == 8)
	{
		uint8_t* samples8 = new uint8_t[fileheader.SUBCHUNK2_SIZE];
		in.read(reinterpret_cast<char*>(samples8), fileheader.SUBCHUNK2_SIZE);
		out.write(reinterpret_cast<char*>(samples8), fileheader.SUBCHUNK2_SIZE);

		delete[] samples8;
	}
	else
	{
		std::cout << fileheader.BITRATE << "-bit bitrate is currently unsupported!";
		std::cout << std::endl;
	}
	std::cout << "Done writing sample values to file!" << std::endl;

	// Generate sine waves
	GenSineWAV("sine1.wav", 2, 400, 15000, 1, 8000, 16);
	GenSineWAV("sine2.wav", 2, 1000, 10000, 1, 8000, 16);

	int channels = 1;
	int seconds = 2;
	int sample_rate = 44100;
	int bitrate = 16;
	size_t num_samples = seconds * sample_rate;
	auto sine1_samples = GenSine(seconds, 400, 15000, sample_rate);
	auto sine2_samples = GenSine(seconds, 1000, 10000, sample_rate);
	auto sine3_samples = std::unique_ptr<int16_t>{ new int16_t[num_samples] };
	for (auto i = 0; i < num_samples; ++i)
	{
		sine3_samples.get()[i] = sine1_samples.get()[i] + sine2_samples.get()[i];
	}
	HEADER_WAV sine3_header = GenWAVHeader(channels, num_samples, sample_rate, bitrate);
	WriteWAV("sine3.wav", sine3_header, reinterpret_cast<char*>(sine3_samples.get()));

	out.close();
	in.close();

	std::cout << "All done!" << std::endl;

	std::cin.get();
	return 0;
}

void WriteWAV(const char* filename, HEADER_WAV header, const char* data)
{
	std::ofstream out(filename, std::ios::out | std::ios::binary);

	out.write(reinterpret_cast<char*>(&header), sizeof(header));
	out.write(data, header.SUBCHUNK2_SIZE);

	out.close();
}

std::unique_ptr<int16_t> GenSine(float seconds, float freq, float amplitude, int sample_rate)
{
	size_t buffer_size = seconds * sample_rate;
	auto samples = std::unique_ptr<int16_t>{ new int16_t[buffer_size] };
	for (auto i = 0; i < buffer_size; ++i)
	{
		samples.get()[i] = amplitude * std::sin((2.0f * (float)M_PI * freq) / sample_rate * i);
	}

	return samples;
}

void GenSineWAV(const char* filename, float seconds, float freq, float amplitude, int channels, int sample_rate, int bitrate)
{
	std::ofstream out(filename);

	auto sine_samples = GenSine(seconds, freq, amplitude, sample_rate);

	auto num_samples = seconds * sample_rate;
	HEADER_WAV sine_header = GenWAVHeader(channels, num_samples, sample_rate, bitrate);
	WriteWAV(filename, sine_header, reinterpret_cast<char*>(sine_samples.get()));

	out.close();
}

HEADER_WAV GenWAVHeader(int channels, int num_samples, int sample_rate, int bitrate)
{
	HEADER_WAV header;
	header.RIFF[0] = 'R';
	header.RIFF[1] = 'I';
	header.RIFF[2] = 'F';
	header.RIFF[3] = 'F';
	header.WAVE[0] = 'W';
	header.WAVE[1] = 'A';
	header.WAVE[2] = 'V';
	header.WAVE[3] = 'E';
	header.fmt[0] = 'f';
	header.fmt[1] = 'm';
	header.fmt[2] = 't';
	header.fmt[3] = ' ';
	header.data[0] = 'd';
	header.data[1] = 'a';
	header.data[2] = 't';
	header.data[3] = 'a';

	int bytes = bitrate / 8;

	header.CHANNELS = channels;
	header.SAMPLES = sample_rate;
	header.BYTES_PER_SEC = sample_rate * channels * bytes; // * (bitrate / 8.0f);
	header.BLOCK_ALIGN = channels * bytes;
	header.BITRATE = bitrate;
	header.SUBCHUNK2_SIZE = num_samples * channels * bytes;

	header.AUDIO_FORMAT = 1;

	header.SUBCHUNK1_SIZE = 16;
	header.CHUNK_SIZE = 36 + header.SUBCHUNK2_SIZE;

	return header;
}

void PrintWAVHeader(HEADER_WAV fileheader)
{
	std::cout << "ChunkID: "		<< std::string(reinterpret_cast<char*>(fileheader.RIFF), 4) << std::endl;
	std::cout << "ChunkSize: "		<< fileheader.CHUNK_SIZE									<< std::endl;
	std::cout << "Format: "			<< std::string(reinterpret_cast<char*>(fileheader.WAVE), 4) << std::endl;
	std::cout << "Subchunk1ID: "	<< std::string(reinterpret_cast<char*>(fileheader.fmt), 4)	<< std::endl;
	std::cout << "Subchunk1Size: "	<< fileheader.SUBCHUNK1_SIZE								<< std::endl;
	std::cout << "AudioFormat: "	<< fileheader.AUDIO_FORMAT									<< std::endl;
	std::cout << "NumChannels: "	<< fileheader.CHANNELS										<< std::endl;
	std::cout << "SampleRate: "		<< fileheader.SAMPLES										<< std::endl;
	std::cout << "ByteRate: "		<< fileheader.BYTES_PER_SEC									<< std::endl;
	std::cout << "BlockAlign: "		<< fileheader.BLOCK_ALIGN									<< std::endl;
	std::cout << "BitsPerSample: "	<< fileheader.BITRATE										<< std::endl;
	std::cout << "Subchunk2ID: "	<< std::string(reinterpret_cast<char*>(fileheader.data), 4) << std::endl;
	std::cout << "Subchunk2Size: "	<< fileheader.SUBCHUNK2_SIZE								<< std::endl;
}