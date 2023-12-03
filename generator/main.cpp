#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <numbers>


namespace fs = std::filesystem;
using namespace std::numbers;


// helpers
namespace {
	const std::string INDENT = "    ";

	// trim from start (in place)
	inline void ltrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
		}));
	}

	// trim from end (in place)
	inline void rtrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
		}).base(), s.end());
	}

	// trim from both ends (in place)
	inline void trim(std::string &s) {
		rtrim(s);
		ltrim(s);
	}

	// pase integer, also supports hex (such as 0x90)
	inline int toInt(const std::string &s) {
		int base = 10;
		int i = 0;
		if (s.substr(0, 2) == "0x") {
			base = 16;
			i = 2;
		}

		int value = 0;
		for (; i < s.length(); ++i) {
			value *= base;
			char ch = s[i];

			if (ch >= '0' && ch <= '9') {
				value += ch - '0';
			} else if (base == 16) {
				if (ch >= 'A' && ch <= 'F')
					value += ch - 'A' + 10;
				else if (ch >= 'a' && ch <= 'f')
					value += ch - 'a' + 10;
			}
		}
		return value;
	}

	std::string flt(float value) {
		std::stringstream ss;
		ss << value;
		if (ss.str().find_first_of(".e") == std::string::npos)
			ss << ".0";
		ss << 'f';
		return ss.str();
	}
}

/*
// integer variant
struct IndexColor {
	int index;

	int r;
	int g;
	int b;

	IndexColor() = default;
	IndexColor(int index, int r, int g, int b)
		: index(index), r(r), g(g), b(b) {}
};

void interpolate(std::ofstream & f, const std::string &indent, int index1, int index2, int value1, int value2) {
	// first value
	f << value1;

	int valueDifference = value2 - value1;
	if (valueDifference != 0) {
		// interpolate towards second value
		f << (valueDifference > 0 ? " + " : " - ");
		f << '(';
		f << "(x - 0x" << std::hex << index1 << std::dec << ')';
		f << " * " << std::abs(valueDifference);

		// divide by index difference
		int indexDifference = index2 - index1;
		if ((indexDifference & (indexDifference - 1)) == 0) {
			// can use shift
			int shift = 0;
			while (indexDifference > 1) {
				++shift;
				indexDifference >>= 1;
			}
			f << " >> " << shift << ')';
		} else {
			// use divide
			f << ") / " << indexDifference;
		}
	}
}

void subdivide(std::ofstream & os, const std::string& indent, std::vector<IndexColor> const & palette, int begin, int end) {
	if (end - begin > 1) {
		int mid = (begin + end) / 2;

		os << indent << "if (x < 0x" << std::hex << palette[mid].index << std::dec << ") {" << std::endl;
		subdivide(os, indent + '\t', palette, begin, mid);
		os << indent << "} else {" << std::endl;
		subdivide(os, indent + '\t', palette, mid, end);
		os << indent << "}" << std::endl;
	} else {
		IndexColor color1 = palette[begin];
		IndexColor color2 = palette[begin + 1];

		// add index range as comment
		os << indent << "// 0x" << std::hex << color1.index << " - 0x" << (color2.index - 1) << std::dec << std::endl;

		// interpolate colors
		os << indent << "r = "; interpolate(os, indent, color1.index, color2.index, color1.r, color2.r); os << ";" << std::endl;
		os << indent << "g = "; interpolate(os, indent, color1.index, color2.index, color1.g, color2.g); os << ";" << std::endl;
		os << indent << "b = "; interpolate(os, indent, color1.index, color2.index, color1.b, color2.b); os << ";" << std::endl;
	}
}

// Generate an include file for a palette from a .palette file
void generatePalette(const fs::path &path, const fs::path &outputPath, const std::string &functionName) {
	std::vector<IndexColor> palette;

	// read palette
	std::ifstream is(path.string());
	std::string line;
	while (std::getline(is, line)) {
		// remove comment
		size_t pos = line.find("//");
		if (pos != std::string::npos)
			line = line.substr(0, pos);

		// trim
		trim(line);

		// split
		std::vector<std::string> elements;
		//boost::split(elements, line, boost::algorithm::is_any_of("\t "), boost::token_compress_on);

		while ((pos = line.find_first_of(" \t")) != std::string::npos) {
			elements.push_back(line.substr(0, pos));
			line.erase(0, line.find_first_not_of(" \t", pos));
			//trim(line);
		}
		if (!line.empty())
			elements.push_back(line);


		if (elements.size() >= 2) {

			int index = toInt(elements[0]);


			if (elements.size() == 2) {
				if (elements[1] == "loop") {
					IndexColor ic = palette.front();
					ic.index = index;
					palette.push_back(ic);
				}
			} else if (elements.size() == 4) {
				int r = int(std::round(std::stof(elements[1]) * 4095));
				int g = int(std::round(std::stof(elements[2]) * 4095));
				int b = int(std::round(std::stof(elements[3]) * 4095));

				palette.push_back(IndexColor(index, r, g, b));
			}
		}
	}

	std::ofstream f(outputPath);
	f << "#pragma once" << std::endl;
	f << std::endl;
	f << "inline Color<int> " << functionName << "(int x) {" << std::endl;

	f << "\tint r, g, b;" << std::endl;
	subdivide(f, "\t", palette, 0, palette.size()-1);
	f << "\treturn {r, g, b};" << std::endl;

	f << "}" << std::endl;
	f.close();
}
*/

// float variant
struct IndexColor {
	float index;

	float r;
	float g;
	float b;

	IndexColor() = default;
	IndexColor(float index, float r, float g, float b)
		: index(index), r(r), g(g), b(b) {}
};

void interpolate(std::ofstream &f, const std::string &indent, float index1, float index2, float value1, float value2) {
	// first value
	f << flt(value1);

	// interpolate towards second value if value difference nonzero
	float valueDifference = value2 - value1;
	if (valueDifference != 0) {
		float indexDifference = index2 - index1;
		float s = valueDifference / indexDifference;

		// add or subtract depending on sign
		f << (s > 0 ? " + " : " - ");

		// subtract first index
		if (index1 == 0)
			f << "x";
		else
			f << "(x - " << flt(index1) << ")";

		// multiply by value difference and divide by index difference
		f << " * " << flt(std::abs(s));
	}
}

void subdivide(std::ofstream & os, const std::string& indent, std::vector<IndexColor> const & palette, int begin, int end) {
	if (end - begin > 1) {
		int mid = (begin + end) / 2;

		os << indent << "if (x < " << palette[mid].index << "f) {" << std::endl;
		subdivide(os, indent + INDENT, palette, begin, mid);
		os << indent << "} else {" << std::endl;
		subdivide(os, indent + INDENT, palette, mid, end);
		os << indent << "}" << std::endl;
	} else {
		IndexColor color1 = palette[begin];
		IndexColor color2 = palette[begin + 1];

		// add index range as comment
		os << indent << "// " << color1.index << " - " << color2.index << std::endl;

		// interpolate colors
		os << indent << "r = "; interpolate(os, indent, color1.index, color2.index, color1.r, color2.r); os << ";" << std::endl;
		os << indent << "g = "; interpolate(os, indent, color1.index, color2.index, color1.g, color2.g); os << ";" << std::endl;
		os << indent << "b = "; interpolate(os, indent, color1.index, color2.index, color1.b, color2.b); os << ";" << std::endl;
	}
}

// Generate an include file for a palette from a .palette file
void generatePalette(const fs::path &path, const fs::path &outputPath, const std::string &functionName) {
	std::vector<IndexColor> palette;

	// read palette
	std::ifstream is(path.string());
	std::string line;
	while (std::getline(is, line)) {
		// remove comment
		size_t pos = line.find("//");
		if (pos != std::string::npos)
			line = line.substr(0, pos);

		// trim
		trim(line);

		// split
		std::vector<std::string> elements;
		//boost::split(elements, line, boost::algorithm::is_any_of("\t "), boost::token_compress_on);

		while ((pos = line.find_first_of(" \t")) != std::string::npos) {
			elements.push_back(line.substr(0, pos));
			line.erase(0, line.find_first_not_of(" \t", pos));
			//trim(line);
		}
		if (!line.empty())
			elements.push_back(line);


		if (elements.size() >= 2) {

			float index = std::stof(elements[0]);

			if (elements.size() == 2) {
				if (elements[1] == "loop") {
					IndexColor ic = palette.front();
					ic.index = index;
					palette.push_back(ic);
				}
			} else if (elements.size() == 4) {
				float r = std::stof(elements[1]);
				float g = std::stof(elements[2]);
				float b = std::stof(elements[3]);

				palette.push_back(IndexColor(index, r, g, b));
			}
		}
	}

	std::ofstream f(outputPath);
	f << "#pragma once" << std::endl;
	f << std::endl;
	f << "inline float3 " << functionName << "(float x) {" << std::endl;

	f << INDENT << "float r, g, b;" << std::endl;
	subdivide(f, INDENT, palette, 0, palette.size()-1);
	f << INDENT << "return {r, g, b};" << std::endl;

	f << "}" << std::endl;
	f.close();
}


/**
 * Unsigned cosine with 8 bit output and 10 bit input, shifted to positive values
 */
void generateCosTable(const fs::path &outputPath) {
	std::ofstream f(outputPath);
	f << "#pragma once" << std::endl;
	f << std::endl;

	f << "const uint16_t cos8u10[1024] = {" << std::endl;
	for (int j = 0; j < 64; ++j) {
		f << INDENT;
		for (int i = 0; i < 16; ++i) {
			double x = double(i + 16*j) / 512.0 * pi;
			//int y = (i | j) == 0 ? 255 : 0;
			int y = int(round((cos(x) + 1.0) * 127.5));
			f << y << ", ";
		}
		f << std::endl;
	}
	f << "};" << std::endl;
	//f << "inline uint8_t cos8u10(uint16_t x) {return cos8Table[x];}" << std::endl;
}

int main(int argc, const char **argv) {

	// convert palettes (.palette files) in current effects
	fs::path effectsDir = "src/effects";
	for (auto & entry : fs::directory_iterator(effectsDir)) {
		fs::path path = entry.path();
		std::cout << path.string() << std::endl;
		std::string ext = path.extension().string();
		if (ext == ".palette") {
			std::string functionName = "lookup" + path.stem().string() + "Palette";
			fs::path outputPath = path.parent_path() / (functionName + ".hpp");
			//outputPath.replace_extension(".palette.hpp");
			generatePalette(path, outputPath, functionName);
		}
	}

	generateCosTable(effectsDir / "cos8u10.hpp");

	return 0;
}
