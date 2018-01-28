#pragma once
#include "json.hpp"

namespace TI {
struct fileitem {
	std::string filename;
	std::string md5;
	std::string url;
};

struct RemoteFile {
  const char *filename;
  FILE *stream;
};

void to_json(nlohmann::json& j, const fileitem& p) {
    j = nlohmann::json{{"filename", p.filename}, {"md5", p.md5}, {"url", p.url}};
}

void from_json(const nlohmann::json& j, fileitem& p) {
    p.filename = j.at("filename").get<std::string>();
    p.md5 = j.at("md5").get<std::string>();
    p.url = j.at("url").get<std::string>();
}
}
