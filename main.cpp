#include <iostream>
#include <serial/serial.h>
#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>
#include <algorithm>
using namespace std;

void enumerate_ports()
{
    vector<serial::PortInfo> devices_found = serial::list_ports();

    vector<serial::PortInfo>::iterator iter = devices_found.begin();

    while( iter != devices_found.end() )
    {
        serial::PortInfo device = *iter++;

        printf( "(%s, %s, %s)\n", device.port.c_str(), device.description.c_str(),
                device.hardware_id.c_str() );
    }
}

class Timer {
public:
    Timer(){
        restart();
    }

    double elapsed_sec() const{
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                clock.now() - t0).count() * 0.001;
    }

    std::chrono::milliseconds::rep elapsed_ms() const{
        return std::chrono::duration_cast<std::chrono::milliseconds>(clock.now() - t0).count();
    }

    void restart(){
        t0 = clock.now();
    }
protected:
    std::chrono::system_clock clock;
    std::chrono::system_clock::time_point t0;
};

class Frequency:protected Timer{
public:
    struct Reseter{
        Reseter(unsigned int& cnt)
                :counter(cnt)
        { }
        ~Reseter(){counter = 0;}

        unsigned int &counter;
    };

    Frequency(){
        t0 = elapsed_ms();
    }

    double getFreq(int count = 1){
        counter += count;
        auto t1 = elapsed_ms();
        auto dt = t1 - t0;

        if(dt > 1000){
            t0 = t1;
            freq = counter / (dt * 0.001);
            counter = 0;
        }
        return freq;
    }

    double freq = 0.0;
    std::chrono::milliseconds::rep t0;
    unsigned int counter = 0;
};

std::atomic_int running{1};

void signal_handler(int sig){
    running = 0;
}

int main(int argc, char** argv) {
    if(argc != 3){
        std::cout<<"Usage : xbee_test PORT BAUD"<<std::endl;
        std::cout<<"Example : xbee_test COM1 9600"<<std::endl;
        enumerate_ports();
        return 0;
    }

    std::string port(argv[1]);
    int baud = std::stoi(argv[2]);
    int sim_time_sec = 60;
    std::cout<<"Opening "<<port<<" with "<<baud<<" bps"<<std::endl;
    serial::Serial xbee(port, baud);

    if(!xbee.isOpen()){
        std::cout<<"Cannot open"<<std::endl;
        return 0;
    }
    std::cout<<port<<" is opend"<<std::endl;

    const size_t BUFFER_SIZE = 1000;
    unsigned int sim_time = sim_time_sec * 1000;
    unsigned int count;
    unsigned int stamp_sender;
    unsigned int stamp_recver;
    std::string message;

    std::signal(SIGINT, signal_handler);
    xbee.setTimeout(0,1000,0,0,0);
    Frequency freq;
    Timer clock;

    int expected_size = 40 * (sim_time_sec+10);
    std::vector<unsigned int> v_count; v_count.reserve(expected_size);
    std::vector<unsigned int> v_stamp_sender; v_stamp_sender.reserve(expected_size);
    std::vector<unsigned int> v_stamp_recver; v_stamp_recver.reserve(expected_size);
    std::vector<std::string> v_message; v_message.reserve(expected_size);
    while(running){
        std::string packet = xbee.readline(BUFFER_SIZE);
        stamp_recver = clock.elapsed_ms();
        if(packet.empty()){
            std::cerr<<"Timeout!"<<std::endl;
            return 0;
        }
        while(true){
            size_t pos = packet.find_first_of(',');
            if(pos == std::string::npos)
                break;
            packet[pos] = ' ';
        }
        std::istringstream iss(packet);
        iss>>count>>stamp_sender>>message;
        std::cerr<<count<<" "<<stamp_sender<<" "<<stamp_recver
        <<" "<<message<<std::endl;

        v_count.push_back(count);
        v_stamp_sender.push_back(stamp_sender);
        v_stamp_recver.push_back(stamp_recver);
        v_message.push_back(message);
        if(stamp_recver > sim_time){
            std::cout<<stamp_recver<<" milli-seconds have elapsed."<<std::endl;
            break;
        }
    }
    std::cout<<v_count.size()<<" packet were received"<<std::endl;



    std::cout<<"Bye Bye~"<<std::endl;
    return 0;
}