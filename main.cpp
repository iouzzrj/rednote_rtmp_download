#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

struct HeaderConfig
{
	std::string name;
	std::string value;
};

struct RequestConfig
{
	std::string base_url;
	std::vector<HeaderConfig> headers;
	long timeout_seconds = 30;
};

struct DownloadConfig
{
	std::string base_stream_url = "rtmp://live.xhscdn.com/live/";
	fs::path downloads_root = fs::path{ "downloads" };
	std::string filename_suffix = "_rtmp";
};

struct ProgramConfig
{
	fs::path rtmpdump_exe = "C:/Program Files/RTMPDump/rtmpdump.exe";
};

struct Config
{
	std::string host_id;
	RequestConfig request;
	DownloadConfig download;
	ProgramConfig programs;
};

namespace
{

	std::string read_file(const fs::path& path)
	{
		std::ifstream input(path, std::ios::binary);
		if (!input)
		{
			std::ostringstream oss;
			oss << "无法打开配置文件: " << path;
			throw std::runtime_error(oss.str());
		}

		std::ostringstream buffer;
		buffer << input.rdbuf();
		return buffer.str();
	}

	RequestConfig parse_request(const json& request_json)
	{
		RequestConfig request;
		request.base_url = request_json.at("base_url").get<std::string>();
		if (request_json.contains("timeout_seconds"))
		{
			request.timeout_seconds = request_json.at("timeout_seconds").get<long>();
		}

		if (request_json.contains("headers"))
		{
			const auto& headers_json = request_json.at("headers");
			if (!headers_json.is_object())
			{
				throw std::runtime_error("配置文件中的 headers 字段必须是对象");
			}

			for (const auto& [name, value_json] : headers_json.items())
			{
				HeaderConfig header;
				header.name = name;
				header.value = value_json.get<std::string>();
				request.headers.push_back(std::move(header));
			}
		}

		return request;
	}

	ProgramConfig parse_programs(const json& programs_json)
	{
		if (!programs_json.is_object())
		{
			throw std::runtime_error("配置文件中的 programs 字段必须是对象");
		}

		ProgramConfig programs;

		if (const auto it = programs_json.find("rtmpdump_exe"); it != programs_json.end() && it->is_string())
		{
			programs.rtmpdump_exe = fs::path{ it->get<std::string>() };
		}

		return programs;
	}

	Config parse_config(const fs::path& path)
	{
		const auto file_content = read_file(path);
		auto config_json = json::parse(file_content);

		Config config;
		config.host_id = config_json.at("host_id").get<std::string>();
		config.request = parse_request(config_json.at("request"));
		if (const auto it = config_json.find("programs"); it != config_json.end())
		{
			config.programs = parse_programs(*it);
		}
		return config;
	}

	size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
	{
		auto* stream = static_cast<std::string*>(userdata);
		const auto count = size * nmemb;
		stream->append(ptr, count);
		return count;
	}

	std::string perform_request(const Config& config)
	{
		CURL* curl = curl_easy_init();
		if (!curl)
		{
			throw std::runtime_error("无法初始化 libcurl");
		}

		std::string response;
		struct curl_slist* headers = nullptr;

		std::ostringstream url_stream;
		url_stream << config.request.base_url << "?host_id=" << curl_easy_escape(curl, config.host_id.c_str(), 0);
		const auto url = url_stream.str();

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, config.request.timeout_seconds);
		// 允许 libcurl 自动解压 gzip/deflate 等压缩响应，避免读取到二进制数据
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

		for (const auto& header : config.request.headers)
		{
			std::ostringstream header_stream;
			header_stream << header.name << ": " << header.value;
			headers = curl_slist_append(headers, header_stream.str().c_str());
		}

		if (headers)
		{
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		const auto res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			std::ostringstream error;
			error << "HTTP 请求失败: " << curl_easy_strerror(res);
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			throw std::runtime_error(error.str());
		}

		long status_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);

		if (status_code != 200)
		{
			std::ostringstream error;
			error << "HTTP 响应状态码异常: " << status_code;
			throw std::runtime_error(error.str());
		}

		return response;
	}

        std::optional<std::string> extract_room_id(const json& root)
        {
                const auto dumped = root.dump();
                const std::string target = "room_id";
                std::vector<std::string> room_ids;

                std::size_t search_pos = 0;
                while (true)
                {
                        const auto found = dumped.find(target, search_pos);
                        if (found == std::string::npos)
                        {
                                break;
                        }

                        auto digit_pos = found + target.size();
                        while (digit_pos < dumped.size() && !std::isdigit(static_cast<unsigned char>(dumped[digit_pos])))
                        {
                                ++digit_pos;
                        }

                        if (digit_pos >= dumped.size())
                        {
                                break;
                        }

                        auto end_pos = digit_pos;
                        while (end_pos < dumped.size() && std::isdigit(static_cast<unsigned char>(dumped[end_pos])))
                        {
                                ++end_pos;
                        }

                        if (end_pos > digit_pos)
                        {
                                room_ids.emplace_back(dumped.substr(digit_pos, end_pos - digit_pos));
                        }

                        search_pos = end_pos;
                }

                if (!room_ids.empty())
                {
                        std::cout << "提取到的 room_id 列表: ";
                        for (std::size_t i = 0; i < room_ids.size(); ++i)
                        {
                                if (i > 0)
                                {
                                        std::cout << ", ";
                                }
                                std::cout << room_ids[i];
                        }
                        std::cout << '\n';

                        return room_ids.front();
                }

                std::cout << "未在响应中发现 room_id\n";
                return std::nullopt;
        }

	std::string today_folder_name()
	{
		const auto now = std::chrono::system_clock::now();
		const auto time = std::chrono::system_clock::to_time_t(now);
		std::tm tm{};
#ifdef _WIN32
		localtime_s(&tm, &time);
#else
		localtime_r(&time, &tm);
#endif
		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y%m%d");
		return oss.str();
	}

	fs::path prepare_download_path(const DownloadConfig& download_config, std::string_view room_id)
	{
		fs::path download_dir = download_config.downloads_root / today_folder_name();
		fs::create_directories(download_dir);

		std::string filename = std::string(room_id) + download_config.filename_suffix + ".flv";
		fs::path candidate = download_dir / filename;

		int counter = 1;
		while (fs::exists(candidate))
		{
			std::ostringstream oss;
			oss << room_id << download_config.filename_suffix << '_' << counter << ".flv";
			candidate = download_dir / oss.str();
			++counter;
		}

		return candidate;
	}

	std::string quote_argument(const std::string& arg)
	{
		std::ostringstream oss;
		oss << '"';
		for (const auto ch : arg)
		{
			if (ch == '"')
			{
				oss << '\\';
			}
			oss << ch;
		}
		oss << '"';
		return oss.str();
	}

	std::string quote_argument(const fs::path& path)
	{
		return quote_argument(path.string());
	}

	std::string build_rtmpdump_command(const ProgramConfig& program_config, const std::string& stream_url, const fs::path& output_path)
	{
		std::ostringstream command;
		command << quote_argument(program_config.rtmpdump_exe);
		command << " -r " << quote_argument(stream_url);
		command << " -o " << quote_argument(output_path);
		command << " --live";
		return command.str();
	}

	void trigger_rtmpdump(const Config& config, const std::string& stream_url, const std::string& room_id)
	{
		const auto output_path = prepare_download_path(config.download, room_id);

		std::cout << "下载输出路径: " << output_path << '\n';

		const auto command = build_rtmpdump_command(config.programs, stream_url, output_path);
		std::cout << "rtmpdump 命令: " << command << '\n';

#ifdef _WIN32
		std::cout << "开始调用 rtmpdump 下载 RTMP 流...\n";
		const int exit_code = std::system(command.c_str());
		if (exit_code != 0)
		{
			std::cerr << "rtmpdump 执行失败，退出码: " << exit_code << '\n';
		}
#else
		std::cout << "当前环境不是 Windows，已输出 rtmpdump 命令供手动执行。\n";
#endif
	}

	std::string build_rtmp_url(const Config& config, const std::string& room_id)
	{
		std::ostringstream oss;
		oss << config.download.base_stream_url;
		if (!config.download.base_stream_url.empty() && config.download.base_stream_url.back() != '/')
		{
			oss << '/';
		}
		oss << room_id;
		return oss.str();
	}

} // namespace


int main()
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#else
	std::setlocale(LC_ALL, "");
#endif
	try
	{
		const Config config = parse_config("config.json");
		std::cout << "使用 host_id: " << config.host_id << '\n';

		const auto response = perform_request(config);
		std::cout << "HTTP 响应原始内容: " << response << '\n';

		const auto response_json = json::parse(response);
		const auto room_id = extract_room_id(response_json);

		if (!room_id)
		{
			std::cout << "当前主播没有直播间。\n";
			return 0;
		}

		std::cout << "检测到直播间 room_id=" << *room_id << '\n';

		const auto stream_url = build_rtmp_url(config, *room_id);
		std::cout << "使用固定 RTMP 模板构建播放链接: " << stream_url << '\n';

		trigger_rtmpdump(config, stream_url, *room_id);
	}
	catch (const std::exception& ex)
	{
		std::cerr << "程序异常: " << ex.what() << '\n';
		return 1;
	}

	return 0;
}
