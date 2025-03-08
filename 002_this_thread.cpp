#include <iostream>
#include <thread>
#include <windows.h>
#include <string>
#include <sstream>
#include <chrono>

namespace {
    void tick(int n, std::thread::id cthid){
        std::stringstream ss;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ss << "sleep for thread: " << cthid << "\n";
        std::cout << ss.str();  //
    }

    void clientHandler(int clientId) {
        // Use stringstream to convert thread::id to string
        std::stringstream ss;
        std::thread::id cthid = std::this_thread::get_id();
        ss << "Поток участника " << clientId << ": ID = " << cthid << "\n";
        std::cout << ss.str();  // Output as a single operation
        tick(3, cthid);
    }

    void serverThread() {
        // Use stringstream for server thread ID
        std::stringstream ss;
        std::thread::id cthid = std::this_thread::get_id();
        ss << "Поток сервера: ID = " << cthid << std::endl;
        std::cout << ss.str();
        tick(3, cthid);
        
        // Create client threads
        std::thread client1(clientHandler, 1);
        std::thread client2(clientHandler, 2);

        // Получаем ID созданных клиентских потоков
        std::thread::id client1_id = client1.get_id();
        std::thread::id client2_id = client2.get_id();

        std::stringstream ss1;
        ss1 << "В потоке сервера получен ID клиента 1: " << client1_id << std::endl;
        std::cout << ss1.str();
        std::stringstream ss2;
        ss2 << "В потоке сервера получен ID клиента 2: " << client2_id << std::endl;
        std::cout << ss2.str();
        tick(3, cthid);
        
        client1.join();
        client2.join();
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    
    // Use stringstream for main thread ID
    std::stringstream ss;
    std::thread::id cthid = std::this_thread::get_id();
    ss << "Основной поток: ID = " << cthid << std::endl;
    std::cout << ss.str();
    tick(3, cthid);
    
    // Create server thread
    std::thread server(serverThread);
    server.join();
    
    return 0;
}
