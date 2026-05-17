#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <list>
#include <regex>
#include <opencv2/opencv.hpp>
#include <webp/decode.h>
#include "include/json.hpp"
#include <curl/curl.h>
#include <filesystem>
#include <csignal>
#include <atomic>

using json = nlohmann::json;
namespace fs = std::filesystem;

std::string _rawjson = "";
json _yt_jsoninfo;

std::string _raw_thumb = "";
const std::filesystem::path _working_folder = std::filesystem::current_path();
const std::filesystem::path _opt_folder = _working_folder / "Output";
const std::string _thumb_file = "thumb.png";
std::string _flac_file = "";

std::string _ytlink = "";
std::string _title = "";
std::string _rawinfo = "";



// --- PLATFORM SPECIFIC HEADERS ---
#ifdef _WIN32
    #include <windows.h>
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
    std::wstring to_wstring(const std::string& str) {
        if (str.empty()) return L"";
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }
#endif
void init_unicode_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#else
#endif
}
// --- UNIVERSAL SYSTEM WRAPPER (Fixed Return Type) ---
int run_system_utf8(const std::string& cmd) {
#ifdef _WIN32
    // Return the result of _wsystem so the 'if' statements work
    return _wsystem(to_wstring(cmd).c_str());
#else
    // Return the result of standard system
    return std::system(cmd.c_str());
#endif
}

// --- REDIRECT system() TO OUR WRAPPER ---
#undef system
#define system(x) run_system_utf8(x)
// --- PLATFORM SPECIFIC HEADERS ---



std::string yt_getinfo();
void sel_1();
void sel_2();
int main();

std::atomic<bool> _keepRunning(true);
void handleInterrupt(int signum) {
    _keepRunning = false;
}

std::string check_ytlink(const std::string& _in) {
    std::string _in_mgmt = "";
    if (_in.find("youtu.be/") != std::string::npos) {
        std::cout << "Type: Youtube Share Link." << std::endl;
        for (int i = 0; i < _in.size(); i++) {
            if (_in[i] == '?') {
                break;
            }
            _in_mgmt += _in[i];
        }
    } else if (_in.find("youtube.com/watch?v=") != std::string::npos) {
        if (_in.find("music.youtube.com/watch?v") != std::string::npos) {
            std::cout << "Type: Youtube Music Link." << std::endl;
        } else {
            std::cout << "Type: Youtube Link." << std::endl;
        }
        for (int i = 0; i < _in.size(); i++) {
            if (_in[i] == '&') {
                break;
            }
            _in_mgmt += _in[i];
        }
    } else {
        std::cerr << "Err: Not A Youtube Link or broken Youtube Link. Plz try again." << std::endl;
        _in_mgmt = "";
    }
    return _in_mgmt;
}

void clear_data() {
    _rawjson.clear();
    _yt_jsoninfo.clear();
    _raw_thumb.clear();
    _flac_file.clear();
    _ytlink.clear();
    _title.clear();
    _rawinfo.clear();
}

void tmp_delete() {
    namespace fs = std::filesystem;
    if (fs::exists(_raw_thumb.c_str())) std::filesystem::remove(_raw_thumb.c_str());
    if (fs::exists("raw_thumb.png")) std::filesystem::remove("raw_thumb.png");
    if (fs::exists("thumb.png")) std::filesystem::remove("thumb.png");
    if (fs::exists("raw.info.json")) std::filesystem::remove("raw.info.json");
}

std::string safe_filename(const std::string& s) {
    static const std::regex pattern(R"([<>:\"/\\|?*])");
    return std::regex_replace(s, pattern, "_");
}

cv::Mat decodeWebP(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
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
    if (system(command.c_str()) != 0) {
        std::cerr << "Err: This yt link isn't video link or music link." << std::endl;
        return "";
    }
    
    std::ifstream _json("raw.info.json");
    if (_json.is_open()) {
        std::ostringstream ss;
        ss << _json.rdbuf();
        _rawinfo = ss.str();
        _json.close();
    } else {
        std::cerr << "Error: Could not open raw.info.json" << std::endl;
        return "";
    }

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

    
    std::cout << "_flac_path: " << _flac_path << std::endl << std::endl;
    std::cout << "_new_flac_path: " << _new_flac_path << std::endl << std::endl;

    std::filesystem::path _flac_path_f = _working_folder / _flac_path;
    std::filesystem::remove(_flac_path_f.c_str());
    std::filesystem::path _new_flac_path_f = _working_folder / _new_flac_path;
    std::filesystem::rename(_new_flac_path_f.c_str(), _flac_path.c_str());
    return;
}

void sel_1() {
    std::cout << "Enter youtube link: ";
    std::cin >> _ytlink;
    std::cout << std::endl;

    _ytlink = check_ytlink(_ytlink);

    if (_ytlink == "") {
        return;
    }

    _rawjson = yt_getinfo(_ytlink);
    if (_rawjson == "") {
        return;
    }

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

    tmp_delete();
    return;
}

void sel_2() {
    std::cout << "Enter youtube link: ";
    std::cin >> _ytlink;
    std::cout << std::endl;

    _rawjson = yt_getinfo(_ytlink);
    _yt_jsoninfo = json::parse(_rawjson);
    _yt_jsoninfo["title"] = safe_filename(_yt_jsoninfo["title"]);

    std::string raw_flac = _yt_jsoninfo["title"];
    _flac_file = raw_flac + ".flac";

    yt_getflac(_ytlink);

    std::filesystem::path _out = _opt_folder / _flac_file;
    std::filesystem::copy(_flac_file, _out, std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(_flac_file);

    std::cout << std::endl;
    std::cout << "Title: " << _yt_jsoninfo["title"] << std::endl;
    std::cout << "Image Link: " << _yt_jsoninfo["thumbnail"] << std::endl << std::endl;
    std::cout << "Output File: " << _out << std::endl << std::endl;

    tmp_delete();
    return;
}

void sel_3() {
    std::cout << "Enter youtube link: ";
    std::cin >> _ytlink;
    std::cout << std::endl;

    _rawjson = yt_getinfo(_ytlink);
    _yt_jsoninfo = json::parse(_rawjson);
    _yt_jsoninfo["title"] = safe_filename(_yt_jsoninfo["title"]);

    std::string raw_flac = _yt_jsoninfo["title"];
    _flac_file = raw_flac + ".flac";

    downloadFileCurl(_yt_jsoninfo["thumbnail"]);
    convertToPNG(_raw_thumb, "raw_thumb.png");
    crop_to_square("raw_thumb.png", _thumb_file);

    std::cout << std::endl;
    std::cout << "Image Link: " << _yt_jsoninfo["thumbnail"] << std::endl << std::endl;

    tmp_delete();
    return;
}

void mainusr() {
    std::signal(SIGINT, handleInterrupt);

    std::string disp =
        "=====================================================\n"
        "     YouTube .Flac Audio Downloader (All)\n"
        "=====================================================\n"
        "[1] Download as FLAC with thumbnail (i.ytimg.com)\n"
        "[2] Download as FLAC without thumbnail\n"
        "[3] Download only thumbnail (i.ytimg.com)\n"
        "[4] Download as FLAC with thumbnail (Link + Audio)\n"
        "[5] Download as FLAC with local 'thumb-add' png image\n"
        "[6] Quit\n"
        "=====================================================\n";

    char _in = '7';
    while (_keepRunning) {
        std::cout << disp << std::endl;
        std::cout << "Enter Number: ";

        if (!(std::cin >> _in)) break; 

        switch (_in) {
            case '1':
                sel_1();
                break;
            case '2':
                sel_2();
                break;
            case '3':
                // sel_3(); logic here
                break;
            case '6':
                _keepRunning = false;
                break;
            default:
                std::cout << "Invalid option." << std::endl;
                break;
        }
        
        if (_keepRunning) {
            clear_data();
        }
    }
    std::cout << "\nExiting Program." << std::endl;
}

int main() {
    init_unicode_console(); // For Fixing std::string
    std::cout << "Checking yt-dlp version..." << std::endl;
    if (system("yt-dlp -U") != 0) {
        std::cerr << "Err: Plz Install or Update yt-dlp" << std::endl;
        return 1;
    }

    std::cout << std::endl << "Checking ffmpeg..." << std::endl;
    if (system("ffmpeg -hide_banner -version") != 0) {
        std::cerr << "Err: Plz Install or Update ffmpeg" << std::endl;
        return 1;
    }

    std::cout << std::endl;

    if (!(std::filesystem::exists(_opt_folder) && std::filesystem::is_directory(_opt_folder))) {
        std::filesystem::create_directories(_opt_folder);
    }
    
    tmp_delete();

    mainusr();
    return 0;
}