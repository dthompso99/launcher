#include "launcher.h"
#include <iostream>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>



#define CURL_STATICLIB
#include <curl/curl.h>

#include <zlib.h>

#include "json.hpp"
#include "configure.h"
#include "common.h"
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include <future>
#include <errno.h>

namespace TI {
Launcher::Launcher():_winTitle(PATCHER_TITLE) {
	window = new Fl_Window(300, 200);
	text = new Fl_Box(20, 40, 260, 100, "Fetching Manifest");
	text->box(FL_UP_BOX);
	text->labelsize(20);
	text->labelfont(FL_BOLD + FL_ITALIC);
	text->labeltype(FL_SHADOW_LABEL);
	progress = new Fl_Progress(20,150,260,30);
    progress->minimum(0);
    progress->maximum(1);
    progress->color(0x88888800);
    progress->selection_color(0x4444ff00);
    progress->labelcolor(FL_WHITE);
    progress->label("Initializing....");
	window->end();
}
int Launcher::show(int argc, char **argv) {
	window->show(argc, argv);

	auto fut = std::async(std::launch::async, [&](){
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::cout << "Starting Background Task" << std::endl;
		auto manifest = _fetch_json(makeURL("manifest.json").c_str());
		progress->maximum(manifest.size());
		_total = manifest.size()+1;
		_statusText = "Checking Files";
		checkFiles(manifest);
		std::cout << "Launching " << std::string( EXECUTABLE ).c_str() << std::endl;
		system(std::string( EXECUTABLE ).c_str());
		std::cout << "Finished Launch" << std::endl;
		_done = true;
	});
	while (Fl::wait()) {
		window->copy_label(_winTitle);
		text->copy_label(_statusText);
		progress->value(_progress);
        char progresstext[100];
        sprintf(progresstext, "File %d of %d", _progress, _total);
        progress->copy_label(progresstext);
		//text->redraw();
		if (fut.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready){
			std::cout << "Background process done, we should exit" << std::endl;
			window->remove(text);
			delete(text);
			window->remove(progress);
			delete(progress);
			window->hide();
			delete(window);
			exit(0);
		}
	}
	//auto res = Fl::run();
	return 0;
}
void Launcher::run(){

}

size_t Launcher::WriteCallback(void *buffer, size_t sz, size_t n, void *f) {
	auto host = static_cast<TI::Launcher*>(f);
	size_t realsize = sz * n;
	host->memory = (char*) realloc(host->memory, host->msize + realsize + 1);
	if (host->memory == NULL) {
		return 0;
	}
	memcpy(&(host->memory[host->msize]), buffer, realsize);
	host->msize += realsize;
	host->memory[host->msize] = 0;
	return realsize;
}

size_t Launcher::SaveCallback(void *buffer, size_t sz, size_t n, void *f) {
	struct RemoteFile *out = (struct RemoteFile *) f;
	std::cout << "Writing "<< sz << " " << n << " to " << out->hash << std::endl;
	if (out && !out->stream) {
		/* open file for writing */
		out->stream = fopen(out->hash, "wb");
		if (!out->stream){
			printf ("Error opening file %s: %s\n",out->hash, strerror(errno));
			//std::cout << "unable to open temp file " << out->hash << std::endl;
			return -1; /* failure, can't open file to write */
		}
	}
	return fwrite(buffer, sz, n, out->stream);
}

nlohmann::json Launcher::_fetch_json(const char* url) {
	CURLcode res;
	memory = (char*) malloc(1);
	msize = 0;
	_handle = curl_easy_init();
	curl_easy_setopt(_handle, CURLOPT_URL, url);
	curl_easy_setopt(_handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(_handle, CURLOPT_WRITEFUNCTION,
			TI::Launcher::WriteCallback);
	curl_easy_setopt(_handle, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(_handle, CURLOPT_VERBOSE, true);
	curl_easy_setopt(_handle, CURLOPT_SSL_VERIFYPEER, 0L); //we cant have a crt file on a single file program
	res = curl_easy_perform(_handle);
	curl_easy_cleanup(_handle);
	auto ret = nlohmann::json::parse(memory);
	free(memory);
	return ret;
}

std::string Launcher::makeURL(std::string path) {
	std::string base = MANIFEST_BASE;
	return base + path;
}

int Launcher::decompress(const char* in, const char* out) {
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

int Launcher::compress(const char* in, const char* out) {
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

std::string Launcher::md5(std::string path) {
	std::streampos size;
	char * memblock;
	unsigned long crc = 0L;
	unsigned long c = 0L;

	std::ifstream file(path.c_str(),
			std::ios::in | std::ios::binary | std::ios::ate);
	if (file.is_open()) {
		size = file.tellg();
		memblock = new char[size];
		file.seekg(0, std::ios::beg);
		file.read(memblock, size);
		file.close();
		c = crc32(0, (const Bytef*) memblock, strlen(memblock));
		delete[] memblock;
	}
	return std::to_string(c);
}

bool Launcher::xstat(const char* path) {

#if defined(WIN32) || defined(_WIN32)
	struct _stat64i32 buffer;
	return (_stat(path, &buffer)==0);
#else
	struct stat buffer;
	return (stat(path, &buffer) == 0);
#endif
}

void Launcher::checkFiles(nlohmann::json manifest) {
	for (auto& f : manifest) {
		TI::fileitem file = f;

#if defined(_WIN32)
	int pos2 = file.filename.find_last_of('\\');
	if (pos2 != std::string::npos) {
		_statusText = file.filename.substr(pos2, std::string::npos).c_str();
	} else {
		_statusText = file.filename.c_str();
	}
#else
	int pos2 = file.filename.find_last_of('/');
	if (pos2 != std::string::npos) {
		_statusText = file.filename.substr(pos2, std::string::npos).c_str();
	} else {
		_statusText = file.filename.c_str();
	}
#endif
		if (!xstat(file.filename.c_str()) && md5(file.filename) != file.md5) {
			download(file.md5, file.filename, file.url);
		} else if (file.md5 == std::to_string(0)){
			download(file.md5, file.filename, file.url);
		}
		_progress++;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	_statusText = "Launching";
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void Launcher::download(std::string hash, std::string dest, std::string url) {

#if defined(_WIN32)
	int pos = dest.find_last_of('\\');
	if (pos != std::string::npos) {
		makePath(dest.substr(0, pos));
	}
#else
	int pos = dest.find_last_of('/');
	if (pos != std::string::npos) {
		makePath(dest.substr(0, pos));
	}
#endif
	std::string tmpfile = hash + ".gz";
	//struct RemoteFile f = { tmpfile.c_str(), dest.c_str(), NULL };
	FILE *fp = fopen(tmpfile.c_str(),"wb");
	std::cout << "Fetching " << url << " and saving as " << tmpfile << std::endl;
	_handle = curl_easy_init();
	curl_easy_setopt(_handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(_handle, CURLOPT_FOLLOWLOCATION, 1L);
//	curl_easy_setopt(_handle, CURLOPT_VERBOSE, true);
	curl_easy_setopt(_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(_handle, CURLOPT_WRITEFUNCTION, NULL);
	curl_easy_setopt(_handle, CURLOPT_WRITEDATA, fp);
	auto res = curl_easy_perform(_handle);
	curl_easy_cleanup(_handle);
	if (res > 0){
		fclose(fp);
		_statusText = curl_easy_strerror(res);
	} else {
		fclose(fp);
		decompress(tmpfile.c_str(), dest.c_str());
		remove(tmpfile.c_str());
	}
}

bool Launcher::isDirExist(const std::string& path) {
#if defined(_WIN32)
	struct _stat info;
	if (_stat(path.c_str(), &info) != 0) {
		return false;
	}
	return (info.st_mode & _S_IFDIR) != 0;
#else
	struct stat info;
	if (stat(path.c_str(), &info) != 0) {
		return false;
	}
	return (info.st_mode & S_IFDIR) != 0;
#endif
}

bool Launcher::makePath(const std::string& path) {
#if defined(_WIN32)
	int ret = _mkdir(path.c_str());
#else
	mode_t mode = 0755;
	int ret = mkdir(path.c_str(), mode);
#endif
	if (ret == 0)
		return true;

	switch (errno) {
	case ENOENT:
		// parent didn't exist, try to create it
	{
		int pos = path.find_last_of('/');
		if (pos == std::string::npos)
#if defined(_WIN32)
			pos = path.find_last_of('\\');
			if (pos == std::string::npos)
#endif
			return false;
		if (!makePath(path.substr(0, pos)))
			return false;
	}
		// now, try to create again
#if defined(_WIN32)
		return 0 == _mkdir(path.c_str());
#else
		return 0 == mkdir(path.c_str(), mode);
#endif
	case EEXIST:
		// done!
		return isDirExist(path);
	default:
		return false;
	}
}
}
