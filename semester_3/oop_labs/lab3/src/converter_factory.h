#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Converter.h"
#include "config_parser.h"
#include "audio_stream.h"

class ConverterFactory {
public:
    //указатель на функцию converter
    typedef Converter* (*Creator)(
            const ConfigCommand&,
            const std::vector<WavReader*>&
    );

    //1 раз создается фабрика, потом обращение к ней
    static ConverterFactory* getInstance() {
        static ConverterFactory f;
        return &f;
    }

    //смотрит в конфиге наличие creator по имени
    std::unique_ptr<Converter> create(const ConfigCommand& cmd) const {
        auto it = creators_.find(cmd.name);
        if (it == creators_.end()) {
            return nullptr;
        }
        return std::unique_ptr<Converter>(
                it->second(cmd, inputs_)
        );
    }

    // регистрация
    bool register_converter(const std::string& name, Creator creator) {
        creators_[name] = creator;
        return true;
    }

    //входные wav (для mix)
    void set_inputs(const std::vector<WavReader*>& inputs) {
        inputs_ = inputs;
    }

private:
    ConverterFactory() = default;

    std::map<std::string, Creator> creators_;
    std::vector<WavReader*> inputs_;
};
