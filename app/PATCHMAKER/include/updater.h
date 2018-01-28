#pragma once

#include <vector>
#include <string>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Native_File_Chooser.H>


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

class Updater {
public:
	static Updater* host;
	static void SelectFiles(Fl_Widget *wdgt, void *);
	Updater();
	int show(int argc, char **argv);

    char *memory;
	size_t msize;
	static size_t WriteCallback(void *buffer, size_t sz, size_t n, void *f);
	std::vector<std::string> _files;
	void _appendManifiest(std::string filename, std::string md5, std::string url);
	int compress(const char* in, const char* out);
	int decompress(const char* in, const char* out);
	nlohmann::json _manifest;
private:
	std::string makeURL(std::string path);
	void mkdir(const char* in);
	std::string md5(std::string path);
	void recurseDir(const char* path);
	Fl_Window *window = 0;
	Fl_Native_File_Chooser *G_chooser = 0;
	Fl_Box *text;
	CURL *_handle;
	nlohmann::json _fetch_json(char* url);


};

}
