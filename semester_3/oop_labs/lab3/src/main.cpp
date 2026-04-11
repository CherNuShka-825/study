#include <iostream>
#include <vector>
#include <string>

#include "pipeline.h"
#include "audio_stream.h"
#include "config_parser.h"
#include "converter.h"

static void printUsage(const char* progName) {
    std::cerr << "Usage:\n"
              << "  " << progName << " -c config.txt output.wav input1.wav [input2.wav ...]\n\n"
              << "Where:\n"
              << "  -c config.txt   Path to config file with commands (mute, mix, ...)\n"
              << "  output.wav      Output WAV file (will be created/overwritten)\n"
              << "  input1.wav      Main input stream ($1)\n"
              << "  input2.wav      Optional extra inputs ($2, $3, ...)\n";
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        std::string arg1 = argv[1];
        if (arg1 == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    if (argc < 5) {
        printUsage(argv[0]);
        return 1;
    }

    if (std::string(argv[1]) != "-c") {
        std::cerr << "Error: first argument must be -c\n\n";
        printUsage(argv[0]);
        return 1;
    }

    std::string configPath = argv[2];
    std::string outputPath = argv[3];

    std::vector<std::string> inputFiles;
    inputFiles.reserve(static_cast<std::size_t>(argc - 4));
    for (int i = 4; i < argc; ++i) {
        inputFiles.emplace_back(argv[i]);
    }

    try {
        runPipeline(configPath, outputPath, inputFiles);
    } catch (const ConfigError& e) {
        std::cerr << "Config error: " << e.what() << "\n";
        return 2;
    } catch (const WavReaderError& e) {
        std::cerr << "WAV reader error: " << e.what() << "\n";
        return 3;
    } catch (const WavWriterError& e) {
        std::cerr << "WAV writer error: " << e.what() << "\n";
        return 4;
    } catch (const ConverterError& e) {
        std::cerr << "Converter error: " << e.what() << "\n";
        return 5;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 10;
    }

    return 0;
}
