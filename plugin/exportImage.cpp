#ifndef EXPORT_IMAGE_CPP
#define EXPORT_IMAGE_CPP

#include "exportImage.h"
#include <string>
#include <sstream>

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void exportImage(
	const fs::path &exrImagePath,
	const nlohmann::json &parameters)
{
	if (parameters.at("format").size() > 0)
	{
		float *out;
		int width, height;
		const char *err = NULL;
		LoadEXR(&out, &width, &height, exrImagePath.string().c_str(), &err);

		std::vector<unsigned char> image;
		const int channels = 4;
		image.resize(width * height * channels);
		const float &scale = parameters.at("scale").get<float>();
		const float &gamma = parameters.at("gamma").get<float>();

		const auto hdrToldr = [](const float &hdrPixelValue, const float &gamma_)
		{
			int i = static_cast<int>(255.0f * std::powf(hdrPixelValue, 1.0f / gamma_));
			if (i > 255)
			{
				i = 255;
			}
			if (i < 0)
			{
				i = 0;
			}
			return static_cast<unsigned char>(i);
		};

		// see: https://github.com/syoyo/tinyexr/blob/master/examples/exr2ldr/exr2ldr.cc
		for (int h = 0; h < height; ++h)
		{
			for (int w = 0; w < width; ++w)
			{
				for (int c = 0; c < channels; ++c)
				{
					const int index = c + channels * (w + width * h);
					image.at(index) = hdrToldr(out[index] * scale, (c == 3 ? 1.0 : gamma));
				}
			}
		}

		for (const auto &formatJson : parameters.at("format"))
		{
			const std::string format = formatJson.get<std::string>();

			fs::path imagePath(exrImagePath);
			imagePath = imagePath.parent_path();
			std::string imageFileName(exrImagePath.stem().string());
			imageFileName += ".";
			imageFileName += format;
			imagePath.append(imageFileName);
			if (format == "jpg" || format == "jpeg")
			{
				// currently, quality == 90
				stbi_write_jpg(imagePath.string().c_str(), width, height, channels, &(image[0]), 90);
			}
			else if (format == "png")
			{
				stbi_write_png(imagePath.string().c_str(), width, height, channels, &(image[0]), sizeof(unsigned char) * width * channels);
			}
			else if (format == "bmp")
			{
				stbi_write_bmp(imagePath.string().c_str(), width, height, channels, &(image[0]));
			}
			else if (format == "tga")
			{
				stbi_write_tga(imagePath.string().c_str(), width, height, channels, &(image[0]));
			}
		}
	}

	if (parameters.at("openDirectory").get<bool>())
	{
		// open a directory that containing filePath
		// open output directory.
		std::stringstream cmd;
#if defined(_WIN32) || defined(_WIN64)
		cmd << "start \"\" \"";
#elif defined(__APPLE__)
		cmd << "open \"";
#endif
		cmd << exrImagePath.parent_path().string();
		cmd << "\"";
		system(cmd.str().c_str());
	}
}

#endif
