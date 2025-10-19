#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <ctime>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
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

struct TestModeConfig
{
        bool enabled = false;
        std::string fake_room_id;
};

struct PossibleStartTime
{
        std::string original;
        int minutes_since_midnight = 0;
};

struct PollingConfig
{
        std::vector<PossibleStartTime> possible_start_times;
        int accelerate_offset_minutes = 0;
        int accelerated_wait_minutes = 1;
        int normal_wait_minutes = 5;
        std::vector<std::string> history_start_times;
};

struct Config
{
        std::string host_id;
        RequestConfig request;
        DownloadConfig download;
        ProgramConfig programs;
        TestModeConfig test_mode;
        PollingConfig polling;
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

        std::string trim_copy(std::string_view input)
        {
                while (!input.empty() && std::isspace(static_cast<unsigned char>(input.front())))
                {
                        input.remove_prefix(1);
                }

                while (!input.empty() && std::isspace(static_cast<unsigned char>(input.back())))
                {
                        input.remove_suffix(1);
                }

                return std::string(input);
        }

        int parse_time_string_to_minutes(std::string_view value)
        {
                const std::string trimmed = trim_copy(value);
                const auto colon_pos = trimmed.find(':');
                if (colon_pos == std::string::npos)
                {
                        throw std::runtime_error("配置文件中的时间必须包含冒号，例如 09:30");
                }

                const std::string hour_str = trim_copy(std::string_view(trimmed).substr(0, colon_pos));
                const std::string minute_str = trim_copy(std::string_view(trimmed).substr(colon_pos + 1));
                if (hour_str.empty() || minute_str.empty())
                {
                        throw std::runtime_error("配置文件中的时间格式不正确，小时或分钟为空");
                }

                int hour = 0;
                int minute = 0;
                try
                {
                        hour = std::stoi(hour_str);
                        minute = std::stoi(minute_str);
                }
                catch (const std::exception&)
                {
                        throw std::runtime_error("配置文件中的时间格式不正确，无法解析小时或分钟");
                }

                if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
                {
                        throw std::runtime_error("配置文件中的时间超出有效范围 (00:00-23:59)");
                }

                return hour * 60 + minute;
        }

        int parse_int_field(json& object, std::string_view key, int default_value)
        {
                const std::string key_str(key);
                const auto it = object.find(key_str);
                if (it == object.end())
                {
                        object[key_str] = default_value;
                        return default_value;
                }

                if (!it->is_number_integer())
                {
                        throw std::runtime_error("配置文件中的 " + key_str + " 字段必须是整数");
                }

                const int value = it->get<int>();
                object[key_str] = value;
                return value;
        }

        PollingConfig parse_polling_config(json& config_json)
        {
                PollingConfig polling;

                const std::string start_times_key = "likely_broadcast_times";
                if (!config_json.contains(start_times_key))
                {
                        config_json[start_times_key] = json::array();
                }

                json normalized_start_times = json::array();
                const auto& start_times_json = config_json.at(start_times_key);
                if (!start_times_json.is_array())
                {
                        throw std::runtime_error("配置文件中的 likely_broadcast_times 字段必须是字符串数组");
                }

                for (const auto& value : start_times_json)
                {
                        if (!value.is_string())
                        {
                                throw std::runtime_error("配置文件中的 likely_broadcast_times 条目必须是字符串");
                        }

                        const std::string raw = value.get<std::string>();
                        const std::string trimmed = trim_copy(raw);
                        if (trimmed.empty())
                        {
                                continue;
                        }

                        const int minutes = parse_time_string_to_minutes(trimmed);
                        polling.possible_start_times.push_back(PossibleStartTime{ trimmed, minutes });
                        normalized_start_times.push_back(trimmed);
                }

                config_json[start_times_key] = normalized_start_times;

                int offset = parse_int_field(config_json, "accelerated_request_offset_minutes", polling.accelerate_offset_minutes);
                if (offset < 0)
                {
                        offset = 0;
                }
                polling.accelerate_offset_minutes = offset;
                config_json["accelerated_request_offset_minutes"] = offset;

                int accelerated_wait = parse_int_field(config_json, "accelerated_wait_minutes", polling.accelerated_wait_minutes);
                if (accelerated_wait <= 0)
                {
                        accelerated_wait = polling.accelerated_wait_minutes;
                }
                polling.accelerated_wait_minutes = accelerated_wait;
                config_json["accelerated_wait_minutes"] = accelerated_wait;

                int normal_wait = parse_int_field(config_json, "normal_wait_minutes", polling.normal_wait_minutes);
                if (normal_wait <= 0)
                {
                        normal_wait = polling.normal_wait_minutes;
                }
                polling.normal_wait_minutes = normal_wait;
                config_json["normal_wait_minutes"] = normal_wait;

                const std::string history_key = "history_live_times";
                if (!config_json.contains(history_key))
                {
                        config_json[history_key] = json::array();
                }

                const auto& history_json = config_json.at(history_key);
                if (!history_json.is_array())
                {
                        throw std::runtime_error("配置文件中的 history_live_times 字段必须是字符串数组");
                }

                for (const auto& entry : history_json)
                {
                        if (!entry.is_string())
                        {
                                throw std::runtime_error("配置文件中的 history_live_times 条目必须是字符串");
                        }

                        polling.history_start_times.push_back(entry.get<std::string>());
                }

                return polling;
        }

        std::tm current_local_tm()
        {
                const auto now = std::chrono::system_clock::now();
                const auto time = std::chrono::system_clock::to_time_t(now);
                std::tm tm{};
#ifdef _WIN32
                localtime_s(&tm, &time);
#else
                localtime_r(&time, &tm);
#endif
                return tm;
        }

        bool is_within_accelerated_window(const PollingConfig& polling, int current_minutes)
        {
                if (polling.possible_start_times.empty() || polling.accelerate_offset_minutes <= 0)
                {
                        return false;
                }

                constexpr int minutes_per_day = 24 * 60;
                for (const auto& start_time : polling.possible_start_times)
                {
                        const int diff = std::abs(current_minutes - start_time.minutes_since_midnight);
                        const int wrapped_diff = std::min(diff, minutes_per_day - diff);
                        if (wrapped_diff <= polling.accelerate_offset_minutes)
                        {
                                return true;
                        }
                }

                return false;
        }

        int determine_wait_minutes(const PollingConfig& polling)
        {
                const std::tm tm = current_local_tm();
                const int current_minutes = tm.tm_hour * 60 + tm.tm_min;
                const int accelerated_wait = std::max(1, polling.accelerated_wait_minutes);
                const int normal_wait = std::max(1, polling.normal_wait_minutes);

                if (is_within_accelerated_window(polling, current_minutes))
                {
                        return accelerated_wait;
                }

                return normal_wait;
        }

        std::string current_timestamp_string()
        {
                const std::tm tm = current_local_tm();
                std::ostringstream oss;
                oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
                return oss.str();
        }

        void append_history_entry(Config& config, json& config_json, const fs::path& config_path, const std::string& timestamp)
        {
                config.polling.history_start_times.push_back(timestamp);

                auto& history_json = config_json["history_live_times"];
                if (!history_json.is_array())
                {
                        history_json = json::array();
                }
                history_json.push_back(timestamp);

                std::ofstream output(config_path, std::ios::binary | std::ios::trunc);
                if (!output)
                {
                        std::ostringstream oss;
                        oss << "无法写入配置文件: " << config_path;
                        throw std::runtime_error(oss.str());
                }

                output << std::setw(2) << config_json << '\n';
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

        TestModeConfig parse_test_mode(const json& test_mode_json)
        {
                if (!test_mode_json.is_object())
                {
                        throw std::runtime_error("配置文件中的 test_mode 字段必须是对象");
                }

                TestModeConfig test_mode;
                if (const auto it = test_mode_json.find("enabled"); it != test_mode_json.end())
                {
                        test_mode.enabled = it->get<bool>();
                }
                if (const auto it = test_mode_json.find("fake_room_id"); it != test_mode_json.end())
                {
                        test_mode.fake_room_id = it->get<std::string>();
                }

                return test_mode;
        }

        Config parse_config(const fs::path& path, json* raw_json = nullptr)
        {
                const auto file_content = read_file(path);
                auto config_json = json::parse(file_content);

                Config config;
                config.host_id = config_json.at("host_id").get<std::string>();
                config.request = parse_request(config_json.at("request"));
                config.polling = parse_polling_config(config_json);
                if (const auto it = config_json.find("programs"); it != config_json.end())
                {
                        config.programs = parse_programs(*it);
                }
                if (const auto it = config_json.find("test_mode"); it != config_json.end())
                {
                        config.test_mode = parse_test_mode(*it);
                }
                if (raw_json)
                {
                        *raw_json = std::move(config_json);
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
                const std::tm tm = current_local_tm();
                std::ostringstream oss;
                oss << std::put_time(&tm, "%Y%m%d");
                return oss.str();
        }

        fs::path prepare_download_path(const DownloadConfig& download_config, std::string_view room_id)
        {
                fs::path download_dir = fs::absolute(download_config.downloads_root) / today_folder_name();
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

#ifdef _WIN32
        std::wstring widen_utf8(std::string_view input)
        {
                if (input.empty())
                {
                        return {};
                }

                const int required = MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0);
                if (required <= 0)
                {
                        throw std::runtime_error("无法将字符串从 UTF-8 转换为 UTF-16");
                }

                std::wstring buffer(static_cast<std::size_t>(required), L'\0');
                const int converted = MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), buffer.data(), required);
                if (converted != required)
                {
                        throw std::runtime_error("转换为 UTF-16 时发生未知错误");
                }

                return buffer;
        }

        std::string narrow_utf8(std::wstring_view input)
        {
                if (input.empty())
                {
                        return {};
                }

                const int required = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
                if (required <= 0)
                {
                        return {};
                }

                std::string buffer(static_cast<std::size_t>(required), '\0');
                const int converted = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), buffer.data(), required, nullptr, nullptr);
                if (converted != required)
                {
                        return {};
                }

                return buffer;
        }

        std::string format_windows_error(DWORD error)
        {
                if (error == 0)
                {
                        return {};
                }

                LPWSTR buffer = nullptr;
                const DWORD length = FormatMessageW(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        nullptr,
                        error,
                        0,
                        reinterpret_cast<LPWSTR>(&buffer),
                        0,
                        nullptr);

                if (length == 0 || buffer == nullptr)
                {
                        return {};
                }

                std::wstring_view message(buffer, length);
                std::string result = narrow_utf8(message);
                LocalFree(buffer);

                while (!result.empty() && (result.back() == '\r' || result.back() == '\n'))
                {
                        result.pop_back();
                }

                return result;
        }
#endif

        void trigger_rtmpdump(const Config& config, const std::string& stream_url, const std::string& room_id)
        {
                const auto output_path = prepare_download_path(config.download, room_id);

                std::cout << "下载输出路径: " << output_path << '\n';

                const auto command = build_rtmpdump_command(config.programs, stream_url, output_path);
                std::cout << "rtmpdump 命令: " << command << '\n';

#ifdef _WIN32
                std::wostringstream command_line_stream;
                const std::wstring exe_path = config.programs.rtmpdump_exe.wstring();
                const std::wstring stream_url_w = widen_utf8(stream_url);
                const std::wstring output_path_w = output_path.wstring();
                command_line_stream << L'"' << exe_path << L'"' << L" -r " << L'"' << stream_url_w << L'"' << L" -o " << L'"' << output_path_w << L'"' << L" --live";
                std::wstring command_line = command_line_stream.str();

                std::vector<wchar_t> command_buffer(command_line.begin(), command_line.end());
                command_buffer.push_back(L'\0');

                STARTUPINFOW startup_info{};
                startup_info.cb = sizeof(startup_info);
                PROCESS_INFORMATION process_info{};

                std::cout << "开始调用 rtmpdump 下载 RTMP 流...\n";

                if (!CreateProcessW(
                            nullptr,
                            command_buffer.data(),
                            nullptr,
                            nullptr,
                            FALSE,
                            0,
                            nullptr,
                            nullptr,
                            &startup_info,
                            &process_info))
                {
                        const DWORD error = GetLastError();
                        std::ostringstream oss;
                        oss << "无法启动 rtmpdump，错误代码: " << error;
                        if (const auto message = format_windows_error(error); !message.empty())
                        {
                                oss << " (" << message << ')';
                        }
                        throw std::runtime_error(oss.str());
                }

                WaitForSingleObject(process_info.hProcess, INFINITE);

                DWORD exit_code = 0;
                if (GetExitCodeProcess(process_info.hProcess, &exit_code) && exit_code != 0)
                {
                        std::cerr << "rtmpdump 执行失败，退出码: " << exit_code << '\n';
                }

                CloseHandle(process_info.hThread);
                CloseHandle(process_info.hProcess);
#else
                std::cout << "当前环境不是 Windows，已输出 rtmpdump 命令供手动执行。\n";
#endif
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
                const fs::path config_path = "config.json";
                json raw_config_json;
                Config config = parse_config(config_path, &raw_config_json);

                if (config.test_mode.enabled)
                {
                        if (config.test_mode.fake_room_id.empty())
                        {
                                throw std::runtime_error("测试模式启用但未提供 fake_room_id");
                        }

                        std::cout << "测试模式已启用，使用假 room_id=" << config.test_mode.fake_room_id << '\n';
                        const auto stream_url = build_rtmp_url(config, config.test_mode.fake_room_id);
                        std::cout << "测试模式构建的 RTMP 链接: " << stream_url << '\n';
                        trigger_rtmpdump(config, stream_url, config.test_mode.fake_room_id);
                        return 0;
                }

                std::cout << "使用 host_id: " << config.host_id << '\n';

                while (true)
                {
                        std::optional<std::string> room_id;
                        try
                        {
                                const auto response = perform_request(config);
                                std::cout << "HTTP 响应原始内容: " << response << '\n';

                                const auto response_json = json::parse(response);
                                room_id = extract_room_id(response_json);

                                if (!room_id)
                                {
                                        std::cout << "当前主播没有直播间。\n";
                                }
                                else
                                {
                                        std::cout << "检测到直播间 room_id=" << *room_id << '\n';
                                }
                        }
                        catch (const std::exception& ex)
                        {
                                std::cerr << "请求或解析阶段异常: " << ex.what() << '\n';
                        }

                        if (room_id)
                        {
                                const auto stream_url = build_rtmp_url(config, *room_id);
                                std::cout << "使用固定 RTMP 模板构建播放链接: " << stream_url << '\n';

                                try
                                {
                                        trigger_rtmpdump(config, stream_url, *room_id);
                                        const std::string timestamp = current_timestamp_string();
                                        std::cout << "记录历史开播时间: " << timestamp << '\n';
                                        append_history_entry(config, raw_config_json, config_path, timestamp);
                                }
                                catch (const std::exception& ex)
                                {
                                        std::cerr << "处理直播间时发生错误: " << ex.what() << '\n';
                                }
                        }

                        const int wait_minutes = determine_wait_minutes(config.polling);
                        std::cout << "等待 " << wait_minutes << " 分钟后重试...\n";
                        std::this_thread::sleep_for(std::chrono::minutes(wait_minutes));
                }
        }
        catch (const std::exception& ex)
        {
                std::cerr << "程序异常: " << ex.what() << '\n';
                return 1;
        }

        return 0;
}
