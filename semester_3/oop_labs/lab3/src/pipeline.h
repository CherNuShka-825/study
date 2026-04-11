#pragma once

#include <string>
#include <vector>


void runPipeline(const std::string& configPath,
                  const std::string& outputFile,
                  const std::vector<std::string>& inputFiles);