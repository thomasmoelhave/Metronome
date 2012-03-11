#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/cstdint.hpp>
#include <cstring>
#include <cmath>

namespace wav {

	typedef boost::uint32_t uint32_t;

struct __attribute__ ((__packed__)) riff_header {
	char ChunkID[4];
	uint32_t ChunkSize;
	char Format[4];
};

struct __attribute__ ((__packed__)) fmt { 
	char Subchunk1ID[4];
	uint32_t Subchunk1Size;
	uint16_t AudioFormat;
	uint16_t NumChannels;
	uint32_t SampleRate;
	uint32_t ByteRate;
	uint16_t BlockAlign;
	uint16_t BitsPerSample;
};

struct __attribute__ ((__packed__)) data {
	char Subchunk2ID[4];
	uint32_t Subchunk2Size;
	//followed by the actual data
};

}
 

struct spec {
	spec(int bpm_, int duration_, std::ostream& output_=std::cout) : bpm(bpm_), duration(duration_), output(output_) { }
	int bpm;
	int duration;
	std::ostream& output;
};


//typedef int32_t sample_t;
//typedef int16_t sample_t;
typedef uint8_t sample_t; //8 bpp is unsigned

sample_t generate_sample(uint32_t global_sample, uint32_t last_beat, uint32_t samples_per_beat)
{
	double distance = (global_sample-last_beat);
	const double base = 100.0;
	double x  = distance/ base;

	if (x > 1.0) {
		//outside of peak, return 0 (or the mid of the range if the type is signed
		if (std::numeric_limits<sample_t>::is_signed) {
			return 0;
		} else {
			//return middle of the range for unsigned values
			return std::numeric_limits<sample_t>::max()/2;
		}
	}

	float v = sinf(x*2*M_PI);
	
	//8bit samples are unsigned
	if (!std::numeric_limits<sample_t>::is_signed) {
		v = (v/2.0)+0.5; //in range [0:1]
		v*=std::numeric_limits<sample_t>::max();
		v = std::max(0.0f,std::min(v,float(std::numeric_limits<sample_t>::max())));
	} else {
		v*= (std::numeric_limits<sample_t>::max() - 100);
		v = std::max(float(std::numeric_limits<sample_t>::min()),std::min(v,float(std::numeric_limits<sample_t>::max())));
	}

	return sample_t(v);
}

void generate_data(const spec& s) {

	

	wav::riff_header rh;
	std::memcpy(rh.ChunkID, "RIFF", 4);
	std::memcpy(rh.Format, "WAVE", 4);

	wav::fmt f;
	std::memcpy(f.Subchunk1ID, "fmt ", 4);
	f.AudioFormat = 1; //PCM
	f.Subchunk1Size = sizeof(wav::fmt)-8;
	assert(f.Subchunk1Size == 16);

	f.NumChannels = 1; // Mono

	f.SampleRate = 32000;
	f.BitsPerSample = sizeof(sample_t)*8;
	f.ByteRate = f.SampleRate * f.NumChannels * f.BitsPerSample/8;
	f.BlockAlign = f.NumChannels * f.BitsPerSample/8;

	

	wav::data d;
	std::memcpy(d.Subchunk2ID, "data", 4);

	//compute number of samples (per channel) in total
	uint32_t samples_per_channel = s.duration*f.SampleRate;

	d.Subchunk2Size = samples_per_channel * f.NumChannels * f.BitsPerSample/8;

	rh.ChunkSize = 4 + (8 + f.Subchunk1Size) + 8 + d.Subchunk2Size;

	s.output.write(reinterpret_cast<char*>(&rh),sizeof(rh));
	s.output.write(reinterpret_cast<char*>(&f),sizeof(f));
	s.output.write(reinterpret_cast<char*>(&d),sizeof(d));

	//memory for one second of data
	sample_t* data = new sample_t[f.SampleRate*f.NumChannels];

	const uint32_t samples_per_beat = (f.SampleRate*60)/double(s.bpm);

	uint32_t last_beat = samples_per_beat/2;

	//sample one second at a time
	for (unsigned sec = 0; sec < s.duration; ++sec) {
		for (uint16_t sample = 0; sample < f.SampleRate; ++sample) {
			for (uint16_t c = 0; c < f.NumChannels; ++c) {
	//			data[sample*f.NumChannels + c] = sample_t( (127.0 + std::sin( (sample/double(f.SampleRate)) * 2*M_PI )) * 127.0 );
	//
	
				uint32_t global_sample = sec*f.SampleRate + sample;

				data[sample*f.NumChannels + c] = generate_sample(global_sample,last_beat, samples_per_beat);

				if (global_sample - last_beat  == samples_per_beat) 
					last_beat = global_sample;
	
			}
		}
		s.output.write(reinterpret_cast<char*>(data),sizeof(sample_t)*f.SampleRate*f.NumChannels);
	}

	delete[] data;

}


int main(int argc, char** argv) {

	unsigned bpm = 170;
	unsigned duration = 60;
	boost::shared_ptr<std::ostream> output;//(&std::cout);


	/*std::ostream& hej = std::cout;
	hej = std::cerr;*/

	namespace po = boost::program_options;
	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help", "produce help message")
	    ("output-file", po::value<std::string>(), "Filename of output WAV file (default: output to standard output)")
	    ("bpm", po::value<unsigned>(&bpm), "Beats per minute")
	    ("duration", po::value<unsigned>(&duration), "Duration (in seconds). If the duration is 0 it will run indefinitely.");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);    

	if (vm.count("help")) {
		std::cerr << desc << "\n";
	    return 1;
	}
		
	std::ofstream output_file;
	if (vm.count("output-file")) {
		std::string filename = vm["output-file"].as<std::string>();
		output_file.open(filename.c_str(), std::ios::binary | std::ios::trunc);
		if (!output_file) {
			std::cerr << "failed to open output file \"" << filename << "\"\n";
			return 1;
		}
	}


	{
		const unsigned max_bpm = 1000;
		const unsigned min_bpm = 1;
		if (bpm < min_bpm || bpm > max_bpm) {
			std::cerr << "invalid argument. Supplied bpm (" << bpm << ") no in the range [" << min_bpm << ", " << max_bpm << "]\n";
			return 1;
		}

	}
	std::cerr << "Beats per minut: " << bpm << "\n";
	if (duration == 0) {
		std::cerr << "Continous play\n";
	} else {
		std::cerr << "Duration: " << duration<< " seconds\n";
	}


	spec s(bpm,duration,output_file.is_open() ? output_file : std::cout);
	
	generate_data(s);


	return 0;
}
