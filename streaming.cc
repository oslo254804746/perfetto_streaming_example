#include "shared_mutex"
#include "iostream"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "thread"
#include "perfetto.h"
#include <android/log.h>

auto char *tag = "PERFEETO_STREAMING";
const int  flush_period = 2000;


#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, tag, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, tag, __VA_ARGS__)

// add surface flinger datasource
void add_surface_flinger_datasource(perfetto::TraceConfig &cfg){
    auto* ds = cfg.add_data_sources()->mutable_config();
    ds->set_name("android.surfaceflinger.frametimeline");
}


// add process stats datasource
void add_process_stats_ds(perfetto::TraceConfig &cfg){
    auto* ds = cfg.add_data_sources()->mutable_config();
    ds->set_name("linux.process_stats");
    ds->set_target_buffer(0);
}


// add ftrace datasource
void add_ftrace_ds(perfetto::TraceConfig &cfg, const char * app_name){
    auto* ds = cfg.add_data_sources()->mutable_config();
    ds->set_name("linux.ftrace");
    perfetto::protos::gen::FtraceConfig ftrace_cfg;
    std::string events[] =   {
            "sched",
            "freq",
            "idle",
            "am",
            "wm",
            "gfx",
            "view",
            "binder_driver",
            "hal",
            "dalvik",
            "camera",
            "input",
            "res",
            "memory"
    };
    for (const auto &item: events){
        ftrace_cfg.add_atrace_categories(item);
    }
    ftrace_cfg.set_symbolize_ksyms(true);
    ftrace_cfg.add_atrace_apps(app_name);
    ds->set_ftrace_config_raw(ftrace_cfg.SerializeAsString());
}

// initialize perfetto ,set datasource and fill policy, set ringbuffer and flush period, not file
perfetto::TraceConfig getTraceConfig(const char * app_name){
    perfetto::TraceConfig cfg;
    auto buffer = cfg.add_buffers();
    buffer->set_size_kb(32768);
    buffer->set_fill_policy(
            perfetto::protos::gen::TraceConfig_BufferConfig_FillPolicy::TraceConfig_BufferConfig_FillPolicy_RING_BUFFER
    );
    cfg.set_statsd_logging(perfetto::protos::gen::TraceConfig_StatsdLogging::TraceConfig_StatsdLogging_STATSD_LOGGING_DISABLED);
    cfg.set_write_into_file(false);
    cfg.set_enable_extra_guardrails(false);
    cfg.set_flush_period_ms(flush_period);
    add_ftrace_ds(cfg, app_name);
    add_process_stats_ds(cfg);
    add_surface_flinger_datasource(cfg);
    return cfg;
}

void flush_handler(bool success){
    if (success) {
        LOGI("flush success");
    } else {
        LOGE("flush failed");
    }
}

void send_message_to_server(int & sock, perfetto::TracingSession::ReadTraceCallbackArgs arg){
    u_long size = arg.size;
    auto cd = send(sock, arg.data, arg.size, 0);
    if (cd < 0){
        LOGE("send data error, code:  %s", strerror(cd));
    }else{
        LOGI("send  %zd byte data ", cd);
    }
}


// send trace-packet, the first 4 byte is net order length, then the tracepacket body
// this version send method is called druing perfetto running
void send_message_to_server_with_order(int & sock, perfetto::TracingSession::ReadTraceCallbackArgs arg){
    uint32_t size = arg.size;
    uint32_t header = htonl(size);
    auto header_cd = send(sock, &header, sizeof(header), 0);
    LOGI("send header  %zd byte", header_cd);
    auto cd = send(sock, arg.data, arg.size, 0);
    if (cd < 0){
        LOGE("send data error, code:  %s", strerror(cd));
    }else{
        LOGI("send  %zd byte  data", cd);
    }
}
// send trace-packet, the first 4 byte is net order length, then the tracepacket body
// this version send method is called after perfetto stopped, the all left packet will be send at the same time
void send_message_to_server_with_order(int &sock, std::vector<char> blocking_data){
    uint32_t size = blocking_data.size();
    uint32_t header = htonl(size);
    auto header_cd = send(sock, &header, sizeof(header), 0);
    LOGI("send header  %zd byte", header_cd);
    auto cd = send(sock, &blocking_data[0], static_cast<std::streamsize>(blocking_data.size()), 0);
    LOGI("send  %zd byte  data", cd);
}

class PerfettoStreaming{
private:
    mutable std::shared_mutex mutex_;
    bool _started = false;
    const char *server_host = "127.0.0.1"; // the host of socket will be send
    const char * app_name = ""; // trace android appName/ packege Identifier..like com.adnroid.chrome
    int server_port; // the porf of socket will be send
    int sock_instance;
    void initialize_perfetto();
    std::unique_ptr<perfetto::TracingSession> session;
    bool get_started();
    void clear_tracing();
    bool set_sock();
    // lambda void to async read perfetto trace packet
    perfetto::TracingSession::ReadTraceCallback readTraceCallback = [&](perfetto::TracingSession::ReadTraceCallbackArgs arg){
        if (arg.size > 0 && arg.data != nullptr){
            send_message_to_server_with_order(this->sock_instance, arg);
        }else{
            LOGE("data is empty, skip send");
        }
    };


public:
    void set_server_host(const char * host);
    void set_server_port(int port);
    void set_app_name(const char * appname);

    void start_tracing();
    int stop_tracing();

    ~PerfettoStreaming();
};

void PerfettoStreaming::set_server_host(const char * host) {
    this->server_host=host;
}

void PerfettoStreaming::set_server_port(int port) {
    this->server_port = port;
}

bool PerfettoStreaming::set_sock() {
    if (this->server_port && this->server_host){

        std::cout << "connect to socket server" << std::endl;
        this->sock_instance = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in server_addr{};
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(this->server_host);
        server_addr.sin_port = htons(this->server_port);
        auto connect_result = connect(this->sock_instance, (struct sockaddr *)&server_addr, sizeof server_addr);
        if (connect_result==0){
            LOGI("connect to target socket success");
            return true;
        }else{
            LOGE("connect to target socket error !!! ");
        };
    }
    LOGE("target socket host/port maybe empty! ");
    return false;
}



void PerfettoStreaming::start_tracing() {
    bool result = this->set_sock();
    if (!result){
        LOGE("can not connect to target socket, skip tracing");
        return;
    }
    LOGI("initialize perfetto");
    this->initialize_perfetto();
    LOGI("start PerfEtto tracing");
    this->session = perfetto::Tracing::NewTrace();
    auto cfg = getTraceConfig(this->app_name);
    this->session->Setup(cfg);
    this->session->StartBlocking();
    this->_started = true;
    while (this->get_started()){
        std::this_thread::sleep_for(std::chrono::milliseconds(flush_period));
        if(this->get_started()){
            this->session->Flush(flush_handler, 0);
            LOGI("read tracing  data");
            this->session->ReadTrace(this->readTraceCallback);
        }
    }
}

void PerfettoStreaming::initialize_perfetto() {
    perfetto::TracingInitArgs args;
    args.backends = perfetto::kSystemBackend;
    perfetto::Tracing::Initialize(args);
}


PerfettoStreaming::~PerfettoStreaming() {
    close(this->sock_instance);
}


int PerfettoStreaming::stop_tracing() {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    _started = false;
    this->clear_tracing();
    LOGI("stop perfetto success");
    return 0;
}

bool PerfettoStreaming::get_started() {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return _started;
}

void PerfettoStreaming::set_app_name(const char *appname) {
    this->app_name = appname;
}

void PerfettoStreaming::clear_tracing() {
    this->session->FlushBlocking();
    this->session->StopBlocking();
    auto left_data = this->session->ReadTraceBlocking();
    send_message_to_server_with_order(this->sock_instance, left_data);
    close(this->sock_instance);
    this->sock_instance= 0;
}