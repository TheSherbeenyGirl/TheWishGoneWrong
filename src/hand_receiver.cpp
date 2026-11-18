#include <iostream>
#include <cstring>
#include <string>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    const int PORT = 5005;

    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (serverSocket < 0)
    {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(
        serverSocket,
        reinterpret_cast<sockaddr*>(&serverAddress),
        sizeof(serverAddress)
    ) < 0)
    {
        std::cerr << "Failed to bind socket\n";
        close(serverSocket);
        return 1;
    }

    std::cout << "Waiting for hand data on port " << PORT << "...\n";

    char buffer[1024];

    while (true)
    {
        ssize_t bytesReceived = recvfrom(
            serverSocket,
            buffer,
            sizeof(buffer) - 1,
            0,
            nullptr,
            nullptr
        );

        if (bytesReceived < 0)
            continue;

        buffer[bytesReceived] = '\0';

        int leftPresent, rightPresent;
        float leftX, leftY, rightX, rightY;

        int parsed = sscanf(
            buffer,
            "%d,%f,%f,%d,%f,%f",
            &leftPresent, &leftX, &leftY,
            &rightPresent, &rightX, &rightY
        );

        if (parsed == 6)
        {
            std::cout
                << "\rLEFT: ";

            if (leftPresent)
                std::cout << leftX << ", " << leftY;
            else
                std::cout << "Not detected";

            std::cout << " | RIGHT: ";

            if (rightPresent)
                std::cout << rightX << ", " << rightY;
            else
                std::cout << "Not detected";

            std::cout << "                    " << std::flush;
        }
    }

    close(serverSocket);
    return 0;
}
