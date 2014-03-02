// PhotoSorter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <array>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

using std::string;

boost::gregorian::date convertDate(std::time_t time) {
	tm tm_date;
	auto err = localtime_s(&tm_date, &time);
	if (err != 0) throw std::exception("Time convertion error");
	return boost::gregorian::date_from_tm(tm_date);
}

boost::filesystem::path constructFilename(boost::filesystem::path targetDir, boost::gregorian::date date, string filename) {
	boost::filesystem::path targetFile(targetDir);
	targetFile += "\\" + std::to_string(date.year());
	targetFile += "\\" +std::to_string(date.month().as_number());
	targetFile += "\\" +std::to_string(date.day().as_number());
	targetFile += "\\" +filename;
	return targetFile;
}


bool filesAreEqual(boost::filesystem::path sourceFile, boost::filesystem::path targetFile) {
	// assume files are equal if their size and modified time is equal
	auto sourceSize = boost::filesystem::file_size(sourceFile);
	auto targetSize = boost::filesystem::file_size(targetFile);
	auto sourceDate = boost::filesystem::last_write_time(sourceFile);
	auto targetDate = boost::filesystem::last_write_time(targetFile);

	return sourceSize == targetSize && sourceDate == targetDate;
}

boost::filesystem::path createUniquePath(boost::filesystem::path file) {
	for(int i=1; i < 256; ++i) {
		boost::filesystem::path newpath = file.parent_path();
		newpath += string("\\");
		newpath += file.stem();
		newpath += "_";
		newpath += std::to_string(i);
		newpath += file.extension();
		if (!boost::filesystem::exists(newpath)) {
			return newpath;
		}
	}
	boost::filesystem::path newpath = file.parent_path();
	newpath += string("\\");
	newpath += file.stem();
	newpath += "_RESOLVE.";
	newpath += file.extension();
	return newpath;
}

void sortFile(boost::filesystem::path sourceFile, boost::filesystem::path targetFile) {
	if (!boost::filesystem::exists(targetFile)) {
		std::cout << "COPY " << sourceFile << "\t->\t" << targetFile << "\t" << std::endl;
		boost::filesystem::create_directories(targetFile.parent_path());
		boost::filesystem::copy_file(sourceFile, targetFile);
	} else if (!filesAreEqual(sourceFile, targetFile)) {
		boost::filesystem::path newpath = createUniquePath(targetFile);
		std::cout << "COPY and RENAME " << sourceFile << "\t->\t" << newpath << "\t" << std::endl;
		boost::filesystem::copy_file(sourceFile, newpath, boost::filesystem::copy_option::overwrite_if_exists);
	} else {
		std::cout << "SKIP " << sourceFile << std::endl;
	}
}


void sortPhoto(string sourceDir, string targetDir) {
	std::array<string, 35> extensions = {
		".bmp",
		".gif",
		".jpg",
		".jpeg",
		".png",
		".psd",
		".pspimage",
		".thm",
		".tif",
		".tiff",
		".yuv",
		".ai",
		".drw",
		".eps",
		".ps",
		".svg",
		".raw",
		".ppm",
		".pgm",
		".pbm",
		".pnm",
		".pfm",
		// Video formats
		".3g2",
		".3gp",
		".asf",
		".asx",
		".avi",
		".flv",
		".mov",
		".mp4",
		".mpg",
		".rm",
		".swf",
		".vob",
		".wmv"
	};

	std::for_each(boost::filesystem::recursive_directory_iterator(sourceDir), boost::filesystem::recursive_directory_iterator(), 
		[&] (const boost::filesystem::directory_entry &de) {
			boost::filesystem::path p = de.path();
			
			if (boost::filesystem::is_regular_file(p)) {
				string filename = p.filename().string();

				string ext = p.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), std::tolower);
				auto fit = std::find(extensions.begin(), extensions.end(), ext);
				if (fit != extensions.end()) {
					
					auto date = convertDate(boost::filesystem::last_write_time(p));
					boost::filesystem::path targetFile = constructFilename(targetDir, date, filename);

					sortFile(p, targetFile);
				} else {
					std::cout << "IGNORE " << p << std::endl;
				}
			}
		});
}


int _tmain(int argc, _TCHAR* argv[])
{
	namespace po = boost::program_options;
	po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "Print help message")
            ("source", po::value<string>(), "Source directory")
			("target", po::value<string>(), "Target directory")
        ;


	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm); 
	
	if (vm.count("help")) {
        std::cout << desc << "\n";
		return false;
    }

	string sourceDir;
	if (vm.count("source")) {
		sourceDir = vm["source"].as<string>();
	} else {
		std::cout << "Error: source parameter is not defined" << std::endl;
		desc.print(std::cout);
		return -1;
	}
	string targetDir;
	if (vm.count("target")) {
		targetDir = vm["target"].as<string>();
	} else {
		std::cout << "Error: target parameter is not defined"<< std::endl;
		desc.print(std::cout);
		return -1;
	}
	
	try
	{
		sortPhoto(sourceDir, targetDir);
	} catch(const boost::filesystem::filesystem_error& ex) {
		std::cerr << "Filesystem error: " << ex.what() << std::endl;
		return -1;
	} catch(const std::exception& e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}

