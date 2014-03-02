#include "stdafx.h"

#include <iostream>
#include <array>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

using std::wstring;

boost::gregorian::date convertDate(std::time_t time) {
	tm tm_date;
	auto err = localtime_s(&tm_date, &time);
	if (err != 0) throw std::exception("Time converting error");
	return boost::gregorian::date_from_tm(tm_date);
}

boost::filesystem::path constructFilename(boost::filesystem::path targetDir, boost::gregorian::date date, wstring filename) {
	boost::filesystem::path targetFile(targetDir);
	targetFile += boost::filesystem::path::preferred_separator + std::to_wstring(date.year());
	targetFile += boost::filesystem::path::preferred_separator + std::to_wstring(date.month().as_number());
	targetFile += boost::filesystem::path::preferred_separator + std::to_wstring(date.day().as_number());
	targetFile += boost::filesystem::path::preferred_separator + filename;
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
		newpath += boost::filesystem::path::preferred_separator;
		newpath += file.stem();
		newpath += "_";
		newpath += std::to_string(i);
		newpath += file.extension();
		if (!boost::filesystem::exists(newpath)) {
			return newpath;
		}
	}
	boost::filesystem::path newpath = file.parent_path();
	newpath += boost::filesystem::path::preferred_separator;
	newpath += file.stem();
	newpath += L"_RESOLVE.";
	newpath += file.extension();
	return newpath;
}

void sortFile(boost::filesystem::path sourceFile, boost::filesystem::path targetFile) {
	if (!boost::filesystem::exists(targetFile)) {
		std::wcout << L"COPY " << sourceFile << L"\t->\t" << targetFile << L"\t\n";
		boost::filesystem::create_directories(targetFile.parent_path());
		boost::filesystem::copy_file(sourceFile, targetFile);
	} else if (!filesAreEqual(sourceFile, targetFile)) {
		boost::filesystem::path newpath = createUniquePath(targetFile);
		std::wcout << L"COPY and RENAME " << sourceFile << L"\t->\t" << newpath << L"\t\n";
		boost::filesystem::copy_file(sourceFile, newpath, boost::filesystem::copy_option::overwrite_if_exists);
	} else {
		std::wcout << L"SKIP " << sourceFile << L"\n";
	}
}

void sortPhoto(wstring sourceDir, wstring targetDir) {
	std::array<wstring, 35> extensions = {
		L".bmp",
		L".gif",
		L".jpg",
		L".jpeg",
		L".png",
		L".psd",
		L".pspimage",
		L".thm",
		L".tif",
		L".tiff",
		L".yuv",
		L".ai",
		L".drw",
		L".eps",
		L".ps",
		L".svg",
		L".raw",
		L".ppm",
		L".pgm",
		L".pbm",
		L".pnm",
		L".pfm",
		// Video formats
		L".3g2",
		L".3gp",
		L".asf",
		L".asx",
		L".avi",
		L".flv",
		L".mov",
		L".mp4",
		L".mpg",
		L".rm",
		L".swf",
		L".vob",
		L".wmv"
	};

	std::for_each(boost::filesystem::recursive_directory_iterator(sourceDir), boost::filesystem::recursive_directory_iterator(), 
		[&] (const boost::filesystem::directory_entry &de) {
			boost::filesystem::path p = de.path();
			
			if (boost::filesystem::is_regular_file(p)) {
				wstring filename = p.filename().wstring();

				wstring ext = p.extension().wstring();
				std::transform(ext.begin(), ext.end(), ext.begin(), std::tolower);
				auto fit = std::find(extensions.begin(), extensions.end(), ext);
				if (fit != extensions.end()) {
					auto date = convertDate(boost::filesystem::last_write_time(p));
					boost::filesystem::path targetFile = constructFilename(targetDir, date, filename);
					sortFile(p, targetFile);
				} else {
					std::wcout << L"IGNORE " << p << std::endl;
				}
			}
		});
}

std::wstring str_to_wstr(const std::string& str) {
	std::wstring out;
	std::string::const_iterator i(str.begin()), end(str.end());

	for(; i != end; ++i) {
		out += std::use_facet<std::ctype<wchar_t>>(std::locale()).widen(*i);
	}
	return out;
}


int _tmain(int argc, _TCHAR* argv[])
{
	namespace po = boost::program_options;
	po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "Print help message")
            ("source", po::value<std::string>(), "Source directory")
			("target", po::value<std::string>(), "Target directory")
        ;
	
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm); 
	
	if (vm.count("help")) {
        std::cout << desc << "\n";
		return false;
    }

	wstring sourceDir;
	if (vm.count("source")) {
		sourceDir = str_to_wstr(vm["source"].as<std::string>());
	} else {
		std::cout << "Error: source parameter is not defined" << std::endl;
		desc.print(std::cout);
		return -1;
	}
	wstring targetDir;
	if (vm.count("target")) {
		targetDir = str_to_wstr(vm["target"].as<std::string>());
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

