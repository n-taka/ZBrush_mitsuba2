#ifndef EXPORT_IMAGE_H
#define EXPORT_IMAGE_H

#if defined(_WIN32) || defined(_WIN64)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "boost/filesystem.hpp"
namespace fs = boost::filesystem;
#endif

#include "nlohmann/json.hpp"

void exportImage(
    const fs::path &exrImagePath,
	const nlohmann::json &parameters);

#endif
