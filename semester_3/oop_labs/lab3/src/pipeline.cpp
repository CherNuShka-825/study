#include "pipeline.h"

#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <cstddef>

#include "audio_stream.h"
#include "config_parser.h"
#include "converter.h"
#include "converter_factory.h"

#define BUFFER_SIZE 4096

void runPipeline(const std::string& configPath,
                 const std::string& outputFile,
                 const std::vector<std::string>& inputFiles)
{
    if (inputFiles.empty()) {
        throw WavReaderError("no input WAV files provided");
    }

    std::vector<std::unique_ptr<WavReader>> readers;
    readers.reserve(inputFiles.size());

    for (const auto& path : inputFiles) {
        readers.push_back(std::make_unique<WavReader>(path));
    }

    std::vector<WavReader*> readerPtrs;
    readerPtrs.reserve(readers.size());
    for (auto& r : readers) {
        readerPtrs.push_back(r.get());
    }

    WavReader& mainReader = *readers[0];
    const WavInfo& mainInfo = mainReader.info();

    ConfigParser parser(configPath);
    std::vector<ConfigCommand> commands = parser.parse();

    ConverterFactory* factory = ConverterFactory::getInstance();
    factory->set_inputs(readerPtrs);

    std::vector<std::unique_ptr<Converter>> converters;
    converters.reserve(commands.size());

    for (const auto& cmd : commands) {
        auto conv = factory->create(cmd);
        if (!conv) {
            throw ConverterError("unknown converter: " + cmd.name);
        }
        converters.push_back(std::move(conv));
    }

    WavWriter writer(outputFile, mainInfo);
    std::vector<std::int16_t> buffer(BUFFER_SIZE);

    while (true) {
        std::size_t readCount = mainReader.readSamples(buffer.data(), BUFFER_SIZE);
        if (readCount == 0) {
            break;
        }
        for (auto& conv : converters) {
            conv->process(buffer.data(), readCount);
        }
        writer.writeSamples(buffer.data(), readCount);
    }

    writer.close();
}
