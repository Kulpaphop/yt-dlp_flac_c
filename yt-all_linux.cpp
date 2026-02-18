#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <list>
#include <regex>
#include <opencv2/opencv.hpp>
#include <webp/decode.h>
//#include <limits>

/*#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/std_image_write.h"*/

#include "lib/json.hpp"
#include <curl/curl.h>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

std::string _rawjson = "";
json _yt_jsoninfo;

std::string _raw_thumb = "";
const std::filesystem::path _opt_folder = std::filesystem::current_path() / "Output";
const std::string _thumb_file = "thumb.png";
std::string _flac_file = "";

std::string _ytlink = "";
std::string _title = "";
std::string _rawinfo = "";

std::string yt_getinfo();
void sel_1();
void sel_2();
int main();

void clear_data() {
    _rawjson.clear();
    _yt_jsoninfo.clear();
    _raw_thumb.clear();
    _flac_file.clear();
    _ytlink.clear();
    _title.clear();
    _rawinfo.clear();
}

std::string safe_filename(const std::string& s) {
    static const std::regex pattern(R"([<>:\"/\\|?*])");
    return std::regex_replace(s, pattern, "_");
}

cv::Mat decodeWebP(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());

    int w = 0, h = 0;
    uint8_t* data = WebPDecodeRGBA(buffer.data(), buffer.size(), &w, &h);
    if (!data) {
        std::cerr << "Failed to decode WebP: " << path << std::endl;
        return cv::Mat();
    }

    cv::Mat rgba(h, w, CV_8UC4, data);

    cv::Mat bgr;
    cv::cvtColor(rgba, bgr, cv::COLOR_RGBA2BGR);

    WebPFree(data);
    return bgr;
}

bool convertToPNG(const std::string& inputPath, const std::string& outputPath) {
    cv::Mat image;

    if (inputPath.size() >= 5 &&
        inputPath.substr(inputPath.size() - 5) == ".webp") {
        image = decodeWebP(inputPath);
    } else {
        image = cv::imread(inputPath, cv::IMREAD_UNCHANGED);
    }

    if (image.empty()) {
        std::cerr << "Error: Could not load image " << inputPath << std::endl;
        return false;
    }

    if (!cv::imwrite(outputPath, image)) {
        std::cerr << "Error: Could not save PNG " << outputPath << std::endl;
        return false;
    }

    std::cout << "Converted " << inputPath << " -> " << outputPath << std::endl;
    return true;
}

void crop_to_square(const std::string& image_path, const std::string& output_path) {
    cv::Mat image;

    if (image_path.size() >= 5 &&
        image_path.substr(image_path.size() - 5) == ".webp") {
        image = decodeWebP(image_path);
    } else {
        image = cv::imread(image_path);
    }

    if (image.empty()) {
        std::cerr << "Failed to load image: " << image_path << std::endl;
        return;
    }

    int width = image.cols;
    int height = image.rows;
    int target_size = std::min(width, height);

    int left = (width - target_size) / 2;
    int top  = (height - target_size) / 2;

    cv::Rect roi(left, top, target_size, target_size);
    cv::Mat cropped = image(roi);

    if (!cv::imwrite(output_path, cropped)) {
        std::cerr << "Error: Could not save cropped PNG " << output_path << std::endl;
    } else {
        std::cout << "Cropped square saved: " << output_path << std::endl;
    }
}


size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream) {
    std::ofstream* out = static_cast<std::ofstream*>(stream);
    out->write(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

std::string getFilenameFromUrl(const std::string& url) {
    size_t pos = url.find_last_of('/');
    if (pos == std::string::npos) return "downloaded_file";
    return url.substr(pos + 1);
}

bool downloadFileCurl(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    std::string destination = getFilenameFromUrl(url);
    std::cout << "file from link name: " << destination << std::endl;
    std::ofstream file(destination, std::ios::binary);
    if (!file.is_open()) {curl_easy_cleanup(curl); return false;}
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    file.close();
    _raw_thumb = destination;
    return res == CURLE_OK;
}

std::string yt_getinfo(const std::string& _in) {
    std::string command = "yt-dlp --skip-download --write-info-json --quiet -o \"raw\" " + _in; // Output to "raw.info.json" file
    system(command.c_str());
    std::ifstream _json(".//raw.info.json");
    if (_json.is_open()) { std::string line; while (std::getline(_json, line)) { _rawinfo += line + "\n"; } _json.close(); }
    return _rawinfo;
}

void yt_getflac(const std::string& _url) {
    std::string _opt = _yt_jsoninfo["title"];
    std::string command = "yt-dlp -x --audio-format flac --audio-quality 0 --add-metadata --no-playlist --postprocessor-args \"ffmpeg:-ar 48000 -ac 2 -vn\" -o \"" + _opt + ".flac\" " + _url;
    system(command.c_str());
    return;
}

void add_flac_cover(const std::string& _flac_path, const std::string& _cover_path) {
    std::string _new_flac_path = "_temp_" + _flac_path;
    std::string command =
        "ffmpeg -i \"" + _flac_path + "\" -i \"" + _cover_path + "\" "
        "-map 0:a -map 1:v -c copy "
        "-disposition:v attached_pic "
        "-metadata:s:v title=\"Album cover\" "
        "-metadata:s:v comment=\"Cover (front)\" "
        "-y \"" + _new_flac_path + "\"";
    system(command.c_str());
    std::remove(_flac_path.c_str());
    std::rename(_new_flac_path.c_str(), _flac_path.c_str());
    return;
}

void sel_1() {
    std::cout << "Enter youtube link: ";
    std::cin >> _ytlink;
    if (_ytlink == "") { 
        std::cout << "Err: Null Link" << std::endl << std::endl;
        return;
    }

    std::cout << std::endl;

    _rawjson = yt_getinfo(_ytlink);
    _yt_jsoninfo = json::parse(_rawjson);
    _yt_jsoninfo["title"] = safe_filename(_yt_jsoninfo["title"]);

    std::string raw_flac = _yt_jsoninfo["title"];
    _flac_file = raw_flac + ".flac";

    downloadFileCurl(_yt_jsoninfo["thumbnail"]);
    convertToPNG(_raw_thumb, "raw_thumb.png");
    crop_to_square("raw_thumb.png", _thumb_file);

    yt_getflac(_ytlink);
    add_flac_cover(_flac_file, _thumb_file);

    std::filesystem::path _out = _opt_folder / _flac_file;
    std::filesystem::copy(_flac_file, _out, std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(_flac_file);

    std::cout << std::endl;
    std::cout << "Title: " << _yt_jsoninfo["title"] << std::endl;
    std::cout << "Image Link: " << _yt_jsoninfo["thumbnail"] << std::endl << std::endl;
    std::cout << "Output File: " << _out << std::endl << std::endl;

    // Clear Temp Files
    std::remove(_raw_thumb.c_str());
    std::remove("raw_thumb.png");
    std::remove("thumb.png");
    std::remove("raw.info.json");
    return;
}

void sel_2() {
    std::cout << "Enter youtube link: " << std::endl;
    std::getline(std::cin, _ytlink);
    std::cout << "You selected option 2 with link: " << _ytlink << std::endl;
    return;
}

void init() {

    if (!(std::filesystem::exists(_opt_folder) && std::filesystem::is_directory(_opt_folder))) {
        std::filesystem::create_directories(_opt_folder);
    }

    return;
}

int main() {
    init();

    std::string disp = 
        "=====================================================\n"
        "    YouTube .Flac Audio Downloader (All)\n"
        "=====================================================\n"
        "[1] Download as FLAC with thumbnail (i.ytimg.com)\n"
        "[2] Download as FLAC without thumbnail\n"
        "[3] Download only thumbnail (i.ytimg.com)\n"
        "[4] Download as FLAC with thumbnail (Link + Audio)\n"
        "[5] Download as FLAC with local 'thumb-add' png image\n"
        "[6] Quit\n"
        "=====================================================\n";

    char _in = '7';
    bool _loop = true;
    while (_loop) {
        std::cout << disp << std::endl;
        std::cout << "Enter Number: ";
        std::cin >> _in;
        if (sizeof(_in) > 1) continue;
        switch (_in) {
            case '1':
                sel_1();
		_loop = true;
                break;
            case '2':
                sel_2();
                _loop = true;
                break;
            case '3':
                
                _loop = true;
                break;
            case '4':
                
                _loop = true;
                break;
            case '5':
                
                _loop = true;
                break;
            case '6':
                std::cout << "Exiting the program." << std::endl;
                _loop = false;
                break;
            default:
                _loop = true;
                break;
        }
        clear_data();
	//std::cout << "\033[2J\033[H";
    }
    return 0;
}
