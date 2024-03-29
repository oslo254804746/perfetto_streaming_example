#include "streaming.cc"
#include <csignal>
volatile bool keep_running = true;


void stop_handler(int sig){
    keep_running = false;
}

int main(int argc, char* argv[]) {
     signal(SIGINT, stop_handler);
    auto  *app = "";
    auto *host = "127.0.0.1";
    int port = 0;
    int opt;
    while ((opt = getopt(argc, argv, "a:h:p:")) != -1) {
        switch (opt) {
            case 'a':
                app = optarg;
                break;
            case 'h':
                host = optarg;
                break;
            case 'p':
                try {
                    std::string string_port = optarg;
                    port = std::stoi(string_port);
                }catch(...){
                    LOGE("Read Port ERROR");
                }
                break;
            default:
                LOGI("Usage: -a app -h host -p port\n");
                return 1;
        }
    }
    std::cout << "App: " << app << "\n";
    std::cout << "Host: " << host << "\n";
    std::cout << "Port: " << port << "\n";
    PerfettoStreaming perfetto_streaming;
    perfetto_streaming.set_server_host(host);
    perfetto_streaming.set_server_port(port);
    perfetto_streaming.set_app_name(app);
    std::thread t1([&]{
        perfetto_streaming.start_tracing();
    });
    while (keep_running)
    {
        sleep(1);
    }
    perfetto_streaming.stop_tracing();
    t1.join();
    return 0;
}
