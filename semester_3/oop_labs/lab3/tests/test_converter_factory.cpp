#include <gtest/gtest.h>

#include "converter_factory.h"

class DummyConverter : public Converter {
public:
    explicit DummyConverter(int v) : value(v) {}

    const char* name() const noexcept override {
        return "dummy";
    }

    void process(std::int16_t*, std::size_t) override {}

    int value;
};

static Converter* create_dummy(
        const ConfigCommand& cmd,
        const std::vector<WavReader*>& inputs
) {
    int v = static_cast<int>(cmd.args.size());
    if (!inputs.empty()) {
        v += 100;
    }
    return new DummyConverter(v);
}

TEST(ConverterFactory, SingletonInstance) {
    ConverterFactory* f1 = ConverterFactory::getInstance();
    ConverterFactory* f2 = ConverterFactory::getInstance();
    EXPECT_EQ(f1, f2);
}

TEST(ConverterFactory, CreateUnknownReturnsNullptr) {
    ConverterFactory* factory = ConverterFactory::getInstance();

    ConfigCommand cmd;
    cmd.name = "unknown";
    cmd.args = {};

    auto conv = factory->create(cmd);
    EXPECT_EQ(conv, nullptr);
}

TEST(ConverterFactory, RegisterAndCreateWorks) {
    ConverterFactory* factory = ConverterFactory::getInstance();

    bool ok = factory->register_converter("dummy", create_dummy);
    EXPECT_TRUE(ok);

    std::vector<WavReader*> inputs;
    inputs.push_back(reinterpret_cast<WavReader*>(0x1));
    factory->set_inputs(inputs);

    ConfigCommand cmd;
    cmd.name = "dummy";
    cmd.args = {"a", "b"};

    auto conv = factory->create(cmd);
    ASSERT_NE(conv, nullptr);

    auto* d = dynamic_cast<DummyConverter*>(conv.get());
    ASSERT_NE(d, nullptr);

    EXPECT_EQ(d->value, 102);
}
