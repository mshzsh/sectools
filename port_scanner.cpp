#include <iostream>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <mutex>
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv;
int active_threads = 0;
const int MAX_THREADS = 200; // Maximum number of concurrent threads
const int START_PORT = 1;
const int END_PORT = 20000;

void check_port(const std::string &ip, int port)
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        ++active_threads;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Error creating socket\n";
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr)) <= 0)
    {
        std::cerr << "Invalid IP address\n";
        return;
    }

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        // Port is closed
    }
    else
    {
        std::cout << "Port " << port << " is open.\n";
        close(sockfd);
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        --active_threads;
    }
    cv.notify_one();
}

int main()
{
    std::string ip;
    std::cout << "Enter IP address: ";
    std::cin >> ip;

    std::vector<std::thread> threads;

    // We'll try to connect to ports in the range [1, 20000]
    for (int port = START_PORT; port <= END_PORT; ++port)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []
                { return active_threads < MAX_THREADS; });
        threads.emplace_back(check_port, ip, port);
    }

    for (auto &thread : threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    return 0;
}
