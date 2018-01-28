#include <fstream>
#include <iostream>

#include <zlib.h>

#include "updater.h"
#include "dir.h"
#include "configure.h"
#include "common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include <errno.h>

namespace TI {

Updater* Updater::host;

void Updater::SelectFiles(Fl_Widget *wdgt, void *) {
	host->G_chooser = new Fl_Native_File_Chooser();
	host->G_chooser->directory(".");
	host->G_chooser->type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
	host->G_chooser->title("Choose Release Files..");
	switch (host->G_chooser->show()) {
	case -1:
		break;	// Error
	case 1:
		break; 	// Cancel
	default:		// Choice
		host->G_chooser->preset_file(host->G_chooser->filename());
		std::cout << "Chosen File" << host->G_chooser->filename() << std::endl;
		host->recurseDir(host->G_chooser->filename());
		for (auto& x : host->_files) {
			std::cout << x << std::endl;
			std::string tmp = x.substr(strlen(host->G_chooser->filename()) + 1,
					std::string::npos) + ".gz";
			std::replace(tmp.begin(), tmp.end(), '\\', '_');
			std::replace(tmp.begin(), tmp.end(), ' ', '_');
			host->_appendManifiest(
					x.substr(strlen(host->G_chooser->filename()) + 1,
							std::string::npos), host->md5(x),
					host->makeURL(tmp));
			host->compress(x.c_str(), std::string("cache/" + tmp).c_str());
		}
		std::ofstream m("cache/manifest.json");
		m << host->_manifest.dump();
		m.close();
		host->text->label("Finished");
		host->text->redraw();
		break;
	}
}

int Updater::compress(const char* in, const char* out) {
	FILE *infile = fopen(in, "rb");
	gzFile outfile = gzopen(out, "wb");
	char inbuffer[128];
	int num_read = 0;
	unsigned long total_read = 0, total_wrote = 0;
	while ((num_read = fread(inbuffer, 1, sizeof(inbuffer), infile)) > 0) {
		total_read += num_read;
		gzwrite(outfile, inbuffer, num_read);
	}
	fclose(infile);
	gzclose(outfile);
	return 1;
}

void Updater::mkdir(const char* in) {
	mode_t nMode = 0733;
	int nError = 0;
#if defined(_WIN32)
	nError = _mkdir(in); // can be used on Windows
#else
	nError = mkdir(in, nMode); // can be used on non-Windows
#endif
}

int Updater::decompress(const char* in, const char* out) {
	gzFile infile = gzopen(in, "rb");
	FILE *outfile = fopen(out, "wb");
	char buffer[128];
	int num_read = 0;
	while ((num_read = gzread(infile, buffer, sizeof(buffer))) > 0) {
		fwrite(buffer, 1, num_read, outfile);
	}

	gzclose(infile);
	fclose(outfile);
	return 1;
}

std::string Updater::md5(std::string path) {
	text->label(path.c_str());
	text->redraw();
	std::streampos size;
	char * memblock;
	unsigned long crc = 0L;
	unsigned long c;

	std::ifstream file(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
	if (file.is_open()) {
		size = file.tellg();
		memblock = new char[size];
		file.seekg(0, std::ios::beg);
		file.read(memblock, size);
		file.close();
		c = crc32(0, (const Bytef*) memblock, strlen(memblock));
		std::cout << "size of " << path << " : " << size << std::endl;
		delete[] memblock;
	} else {
		std::cout << "UNABLE TO OPEN FILE: " << path.c_str() << std::endl;
	}
	if (c == 0) {
		std::cout << "ERROR GETTING CRC: "<< errno << " " << strerror(errno) << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
	}
	return std::to_string(c);
}

void Updater::_appendManifiest(std::string filename, std::string md5,
		std::string url) {
	fileitem f = { filename, md5, url };
	_manifest.push_back(f);
}

void Updater::recurseDir(const char* path) {
	std::cout << "Reading Dir: " << path << std::endl;
	tinydir_dir dir;
	tinydir_open(&dir, path);
	while (dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);
		if (file.name[0] != '.') {
			char newpath[1000];
#if defined(WIN32) || defined(_WIN32)
			sprintf(newpath, "%s\\%s", path, file.name);
#else
			sprintf(newpath, "%s/%s", path, file.name);
#endif
			if (file.is_dir) {
				recurseDir(newpath);
			} else {
				_files.push_back(newpath);
			}
		}
		tinydir_next(&dir);
	}
	tinydir_close(&dir);
}

Updater::Updater() {
	Updater::host = this;

	window = new Fl_Window(300, 180);
	window->label(PATCHER_TITLE);
	text = new Fl_Box(20, 10, 260, 40, "Select Distribution Files");
	//text->box(FL_UP_BOX);
	text->labelsize(20);

	Fl_Button *bs = new Fl_Button(20, 60, 260, 40, "Choose");
	bs->callback(SelectFiles);
	//bs->deactivate();

	window->end();
	mkdir("cache");
	//auto manifest = _fetch_json(MANIFEST_BASE);
	//std::cout << manifest.dump() << std::endl;

}

int Updater::show(int argc, char **argv) {
	window->show(argc, argv);
	return Fl::run();
}

size_t Updater::WriteCallback(void *buffer, size_t sz, size_t n, void *f) {
	auto host = static_cast<Updater*>(f);
	size_t realsize = sz * n;
	host->memory = (char*) realloc(host->memory, host->msize + realsize + 1);
	if (host->memory == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(host->memory[host->msize]), buffer, realsize);
	host->msize += realsize;
	host->memory[host->msize] = 0;

	return realsize;
}

nlohmann::json Updater::_fetch_json(char* url) {
	CURLcode res;
	memory = (char*) malloc(1);
	msize = 0;
	_handle = curl_easy_init();
	curl_easy_setopt(_handle, CURLOPT_URL, url);
	curl_easy_setopt(_handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(_handle, CURLOPT_WRITEFUNCTION, Updater::WriteCallback);
	curl_easy_setopt(_handle, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(_handle, CURLOPT_VERBOSE, true);
	curl_easy_setopt(_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	res = curl_easy_perform(_handle);
	auto ret = nlohmann::json::parse(memory);
	free(memory);
	return ret;
}

std::string Updater::makeURL(std::string path) {
	std::string base = MANIFEST_BASE;
	return base + path;
}

}
