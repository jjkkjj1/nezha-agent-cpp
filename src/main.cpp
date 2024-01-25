#include <string>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>
#include <proto/nezha.grpc.pb.h>
#include <proto/nezha.pb.h>
#include <iostream>
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "machine_info/MachineInfo.h"
#include <filesystem>
#include <getopt.h>
#include "IPManager.h"
#include "common.h"

struct agent_cli_param{
    std::string server;
    std::string client_secret;
    unsigned long long report_delay;
    bool tls;
    agent_cli_param():report_delay(1), tls(false){}
    void show(){
        std::cout << "server:" << server << std::endl;
        std::cout << "client_secret:" << client_secret << std::endl;
        std::cout << "report_delay:" << report_delay << std::endl;
        std::cout << "tls:" << tls << std::endl;
    }
};

int delay_when_error = 10; // Agent 重连间隔
int network_time_out = 5;  // 普通网络超时
agent_cli_param params;
std::shared_ptr<spdlog::logger> logger;

class MyCustomAuthenticator : public grpc::MetadataCredentialsPlugin {
public:
    MyCustomAuthenticator(const grpc::string& ticket) : ticket_(ticket) {}

    grpc::Status GetMetadata(
            grpc::string_ref service_url,
            grpc::string_ref method_name,
            const grpc::AuthContext& channel_auth_context,
            std::multimap<grpc::string, grpc::string>* metadata) override {
        metadata->insert(std::make_pair("client_secret", ticket_));
        return grpc::Status::OK;
    }

private:
    grpc::string ticket_;
};

void InitLog() {
    // 初始化日志
    logger = std::make_shared<spdlog::logger>("multi_sink");

	system("chcp  65001");
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	console_sink->set_level(spdlog::level::debug);
	console_sink->set_pattern("[%H:%M:%S][%^%l%$][%t] %v");
	logger->sinks().push_back(console_sink);

    size_t max_size = 1048576 * 5;
    size_t max_files = 10;
	// 写入utf8 bom
	spdlog::file_event_handlers handlers;
	handlers.after_open = [](const spdlog::filename_t& filename, std::FILE* file_stream) {
		int index = ftell(file_stream);
		fseek(file_stream, 0, SEEK_SET);
		unsigned char buffer[3];
		fgets((char*)buffer, 3, file_stream);
		if (buffer[0] != 0xef || buffer[1] != 0xbb || buffer[2] != 0xbf) {
			fseek(file_stream, 0, SEEK_SET);
			fputc(0xef, file_stream);
			fputc(0xbb, file_stream);
			fputc(0xbf, file_stream);
			index += 3;
		}
		fseek(file_stream, index, SEEK_SET);
	};

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(L"logs/log.txt", max_size, max_files, false, handlers);
	file_sink->set_pattern("[%H:%M:%S][%^%l%$][%t] %v");
	file_sink->set_level(spdlog::level::debug);
	logger->sinks().push_back(file_sink);

    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::debug);
    spdlog::details::registry::instance().register_logger(logger);
}

void ReportStateInfo(){
    grpc::ClientContext context;
    std::chrono::system_clock::time_point deadline = std::chrono::system_clock::now() + std::chrono::seconds(network_time_out);
    context.set_deadline(deadline);

    std::shared_ptr<grpc::ChannelCredentials> channel_creds;
    if (params.tls) {
        auto tls_opts = grpc::SslCredentialsOptions();
        channel_creds = grpc::SslCredentials(tls_opts);
    }
    else {
        channel_creds = grpc::InsecureChannelCredentials();
    }
    auto call_creds = grpc::experimental::MetadataCredentialsFromPlugin(
            std::unique_ptr<grpc::MetadataCredentialsPlugin>(
                    new MyCustomAuthenticator(params.client_secret)), GRPC_SECURITY_NONE);
    context.set_credentials(call_creds);
    auto channel = grpc::CreateChannel(params.server, channel_creds);
    auto stub = proto::NezhaService::NewStub(channel);
    proto::Receipt response1;
    proto::State state;
    state.set_cpu(nezha::MachineInfo::getCPUUsage());
    state.set_mem_used(nezha::MachineInfo::getMemUsage());
    state.set_swap_used(nezha::MachineInfo::getSwapUsage());
    state.set_disk_used(nezha::MachineInfo::getDiskUsage());
    auto transfer = nezha::MachineInfo::getNetTransfer();
    state.set_net_in_transfer(std::get<0>(transfer));
    state.set_net_out_transfer(std::get<1>(transfer));
    auto speed = nezha::MachineInfo::getNetSpeed();
    state.set_net_in_speed(std::get<0>(speed));
    state.set_net_out_speed(std::get<1>(speed));
    state.set_uptime(std::time(0)-nezha::MachineInfo::getMachineLaunchTime());
    auto load = nezha::MachineInfo::getLoadAverage();
    state.set_load1(std::get<0>(load));
    state.set_load5(std::get<1>(load));
    state.set_load15(std::get<2>(load));
    state.set_tcp_conn_count(nezha::MachineInfo::getTcpCount());
    state.set_udp_conn_count(nezha::MachineInfo::getUdpCount());
    auto gpu_rate = nezha::MachineInfo::getGPUUsage();
    auto gpu_rate_int = (int)(gpu_rate * 10000);
    auto process_count = nezha::MachineInfo::getProcessCount();
    auto process_count_with_gpu_rate = gpu_rate_int * 100000 + process_count;
    state.set_process_count(process_count_with_gpu_rate);

    auto status = stub->ReportSystemState(&context, state, &response1);
    if(!status.ok()){
        logger->error("ReportSystemInfo Fail:{}", status.error_message());
    }
}

void ReportSystemInfo(){
    grpc::ClientContext context;
    std::chrono::system_clock::time_point deadline = std::chrono::system_clock::now() + std::chrono::seconds(network_time_out);
    context.set_deadline(deadline);

    std::shared_ptr<grpc::ChannelCredentials> channel_creds;
    if (params.tls) {
        auto tls_opts = grpc::SslCredentialsOptions();
        channel_creds = grpc::SslCredentials(tls_opts);
    }
    else {
        channel_creds = grpc::InsecureChannelCredentials();
    }
    auto call_creds = grpc::experimental::MetadataCredentialsFromPlugin(
            std::unique_ptr<grpc::MetadataCredentialsPlugin>(
                    new MyCustomAuthenticator(params.client_secret)), GRPC_SECURITY_NONE);
    context.set_credentials(call_creds);
    auto channel = grpc::CreateChannel(params.server, channel_creds);
    auto stub = proto::NezhaService::NewStub(channel);
    proto::Host system_info;
    system_info.set_platform(nezha::MachineInfo::getPlatform());
    system_info.set_platform_version(nezha::MachineInfo::getPlatformVersion());
    system_info.set_mem_total(nezha::MachineInfo::getTotalMemory());
    system_info.set_disk_total(nezha::MachineInfo::getTotalDisk());
    system_info.set_swap_total(nezha::MachineInfo::getTotalSwap());
    auto arch = nezha::MachineInfo::getArchitecture();
    auto gpus = nezha::MachineInfo::getGPUs();
    bool first = true;
    for(auto& gpu: gpus){
        if(gpu.memory <= 0) continue;
        if(first){
            first = false;
            arch += std::vformat("]\nGPU：[{} {:.2f}GB",
                                 std::make_format_args(gpu.name, gpu.memory));
        }else{
            arch += std::vformat(", {} {:.2f}GB",
                                 std::make_format_args(gpu.name, gpu.memory));
        }
    }
    system_info.set_arch(arch);
    system_info.set_virtualization("");
    system_info.set_boot_time(nezha::MachineInfo::getMachineLaunchTime());
    system_info.set_ip(nezha::IPManager::GetInstance().getIp());
    system_info.set_country_code(nezha::IPManager::GetInstance().getCountryCode());
    system_info.set_version(VERSION);
    system_info.add_cpu(nezha::MachineInfo::getCPU());

    proto::Receipt response;
    auto status = stub->ReportSystemInfo(&context, system_info, &response);
    if(!status.ok()){
        logger->error("ReportSystemInfo Fail:{}", status.error_message());
    }
}

int main(int argc, char ** argv) {
    auto cur_dir = std::filesystem::current_path();
    SetCurrentDirectoryW(cur_dir.wstring().c_str());
    InitLog();

    nezha::IPManager::GetInstance().run();
    nezha::IPManager::GetInstance().setLogger("multi_sink");
    nezha::MachineInfo::setLogger("multi_sink");
    nezha::MachineInfo::Init();

    const char *short_options = "s:p:R";
    const struct option_a long_options[] = {
            {"server",       required_argument, nullptr, 's'},
            {"password",     required_argument, nullptr, 'p'},
            {"report-delay", required_argument, nullptr, 'R'},
            {nullptr, 0,                        nullptr, 0}  // 结束标志
    };
    int option;
    int option_index;
    while ((option = getopt_long_a(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (option) {
            case 's':
                params.server = optarg_a;
                break;
            case 'p':
                params.client_secret = optarg_a;
                break;
            case 'R':
                params.report_delay = std::stoull(optarg_a);
                break;
        }
    }
    params.show();

    ReportSystemInfo();
    size_t last_info = 0;
    while (true) {
        ReportStateInfo();
        // 每隔十分钟ReportSystemInfo
        if (std::time(nullptr) - last_info >= 600) {
            ReportSystemInfo();
            last_info = std::time(nullptr);
        }
        std::this_thread::sleep_for(std::chrono::seconds(params.report_delay));
    }
    return 0;
}