#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <cerrno>
#include <cstring>
using namespace std;

#define BACKLOG 20 // Bekleyen bağlantı kuyruğu boyutu - Maximum number of pending connections in the queue
#define PORT 5005  // Port numarası

string generateDynamicFilename() {
    static int imageCount = 1;
    string oss;
    oss ="image" + to_string(imageCount) + ".png";
    imageCount++;
    return oss;
}

void handleData(int clientSocket) {
    int channels, width, height;

    while (true) {  // Sürekli olarak yeni bağlantıları dinlemek için döngü
        // Meta verileri al
        recv(clientSocket, &channels, sizeof(int), 0);
        recv(clientSocket, &width, sizeof(int), 0);
        recv(clientSocket, &height, sizeof(int), 0);

        cout << "Kanal sayısı: " << channels << ", Genişlik: " << width << ", Yükseklik: " << height << endl;

        // Görüntü verisini al
        size_t imageSize = width * height * channels;
        cv::Mat receivedImage(height, width, CV_8UC3);

        uchar* imgPtr = receivedImage.data;

        size_t bytesReceived = 0;
        while (bytesReceived < imageSize) {
            int bytes = recv(clientSocket, imgPtr + bytesReceived, imageSize - bytesReceived, 0);
            if (bytes <= 0) {
                std::cerr << "Veri alımı tamamlanamadı veya hata oluştu." << endl;
                break;
            }
            bytesReceived += bytes;
        }

        if (bytesReceived == imageSize && !receivedImage.empty()) {
            string filename = generateDynamicFilename();
            cv::imwrite("/home/ridvanmsl/Desktop/" + filename, receivedImage);
            cout << "Image successfully saved as: " << filename << endl;
        } else {
            cerr << "Failed to receive the image!" << endl;
        }
    }
}

int main() {
    int serverSocket, clientSocket = -1;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Socket oluşturulamadı." << endl;
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Socket bağlanamadı." << endl;
        return -1;
    }

    if (listen(serverSocket, BACKLOG) < 0) {
        std::cerr << "Bağlantılar dinlenemedi." << std::endl;
        return -1;
    }

    while (true) {
        cout << "Bağlantılar dinleniyor..." << std::endl;

        if ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen)) < 0) {
            cerr << "Bağlantı kabul edilemedi: " << strerror(errno) << endl;
            return -1;
        } else {
            cout << "Bağlantı kabul edildi." << endl;
        }

        cout << "İstemci bağlandı." << endl;

        handleData(clientSocket);
    }

    close(serverSocket);

    return 0;
}