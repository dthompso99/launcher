#pragma once

#include <vector>
#include <string>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Progress.H>

#define CURL_STATICLIB
#include <curl/curl.h>

#include "json.hpp"

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
typedef int mode_t;
#else
#define PATH_SEPARATOR "/"
#endif

namespace TI {
class Launcher {
public:
	Launcher();
	int show(int argc, char **argv);
	void run();
	char *memory;
	size_t msize;
	static size_t WriteCallback(void *buffer, size_t sz, size_t n, void *f);
	static size_t SaveCallback(void *buffer, size_t sz, size_t n, void *f);
	int compress(const char* in, const char* out);
	int decompress(const char* in, const char* out);
private:
	Fl_Window *window;
	Fl_Box *text;
	Fl_Progress *progress;
	CURL *_handle;
	nlohmann::json _fetch_json(const char* url);
	std::string makeURL(std::string path);
	std::string md5(std::string path);
	bool xstat(const char* path);
	void checkFiles(nlohmann::json manifest);
	void download(std::string hash, std::string dest, std::string url);
	bool isDirExist(const std::string& path);
	bool makePath(const std::string& path);
	const char* _winTitle;
	const char* _statusText = "Loading";
	int _total = 0;
	int _progress = 1;
	bool _done = false;
};
}
