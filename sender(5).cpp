// g++ sender.cpp -o sender -I/usr/include/opencv4/ -L/usr/lib/x86_64-linux-gnu/ -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs -lpcap
// /home/ridvanmsl/Desktop/test/image1.png
#include <iostream>
#include <pcap.h> // libpcap kütüphanesi
#include <fstream> //Dosya işlemleri için standart C++ kütüphanesi
#include <opencv2/opencv.hpp> // OpenCV kütüphanesi
#include <unistd.h> // Unix system calls
#include <string.h>
#include <cstdint>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <string>

using namespace std;
#define PORT 5005

//bir soket üzerinden OpenCV'nin cv::Mat türündeki bir görüntünün meta verilerini göndermeyi amaçlar
void handleClient(int clientSocket, const char* receiverIP, cv::Mat& image) {

    if (!image.empty()) {
        int channels= image.channels();
        int width= image.cols;
        int height = image.rows;


        // Meta verileri istemciye gönder
        send(clientSocket, &channels, sizeof(int), 0);
        send(clientSocket, &width, sizeof(int), 0);
        send(clientSocket, &height, sizeof(int), 0);
        cout<< "Width: " << width <<endl; 
        cout<< "Height: " << height <<endl;
        cout<< "Channel: " << channels <<endl;
        
        //görüntüyü veri olarak gönder
        int imageSize = image.total() * image.elemSize();
        send(clientSocket, image.data, imageSize, 0);

    
    //Resmin boyutu ve verisi
    imageSize = width * height * channels;
    uchar* imgPtr = image.data;

    int bytesSent = 0;
    while (bytesSent < imageSize) {
        int bytes = send(clientSocket, imgPtr + bytesSent, imageSize - bytesSent,0); //imageSize - bytesSent, 0);
                    cout<< "Byte: " <<bytes << endl;
                    cout<< "Image Size: " <<imageSize << endl;
        if(bytes <= 0) {
            if(bytes== 0){
                cerr << "Bağlantı Kapalı" << std::endl;
            }
            else{
                cerr << "Veri gönderimi tamamlanamadı veya hata oluştu. Hata kodu: " << errno << " - " << strerror(errno) << std::endl;
                cout<< bytes << std::endl;
            }
           
            break;
        }
        bytesSent += bytes;
    }
    }
}


int main() {
    cv::Mat image;
    pcap_if_t *alldevs, *d; // Hata iletileri için bir tampon
    pcap_t *fp;
    char errbuf[PCAP_ERRBUF_SIZE]; 
    //Bu tampon, libpcap kütüphanesinden gelebilecek hata iletilerini tutar

    int clientSocket;
    struct sockaddr_in serverAddr;

    // Kullanıcıdan alıcı IP adresini ve port numarasını al
    std::string receiverIP;
    int receiverPort;
    std::cout << "Alıcı IP adresini girin: ";
    std::cin >> receiverIP;

    // Soket oluştur
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        std::cerr << "Socket oluşturulamadı!" << std::endl;
        return 1;
    }


    // Alıcı adresi ayarla
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(receiverIP.c_str());
    memset(&(serverAddr.sin_zero), '\0', 8);

       // Alıcıya bağlan
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        close(clientSocket);
        return 1;
    }


    std::cout << "\n **** printing the device list: *****\n";

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        std::cerr << "Error in pcap_findalldevs_ex: " << errbuf << std::endl;
        return -1;
    }

    int i = 0;
    for (d = alldevs; d; d = d->next) {
        std::cout << ++i << "  " << d->name << ": ";
        if (d->description)
            std::cout << d->description << std::endl;
        else
            std::cout << "No description available" << std::endl;
    }

    if (i == 0) {
        std::cerr << "No interfaces found! Exiting." << std::endl;
        return -1;
    }

    int inum;
    std::cout << "Enter the interface number (1-" << i << "): ";
    std::cin >> inum;

    if (inum < 1 || inum > i) {
        std::cout << "\nInterface number out of range." << std::endl;
        pcap_freealldevs(alldevs);
        return -1;
    }

    for (d = alldevs, i = 0; i < inum - 1; d = d->next, i++);

    if ((fp = pcap_open_live(d->name, 100, 1, 100, errbuf)) == NULL) {
        std::cerr << "\nError opening adapter\n";
        return -1;
    }

    int packetCount = 1;
    struct pcap_pkthdr *header;
    const u_char *pkt_data;
    int res;
    string empty;
    size_t packetSize;
    while (true) {
        if((res = pcap_next_ex(fp, &header, &pkt_data)) >= 0){
            struct ethhdr *eth = (struct ethhdr *)pkt_data;
            pkt_data += sizeof(struct ethhdr);

            struct iphdr *ip = (struct iphdr *)pkt_data;
            pkt_data += ip->ihl * 4;

            struct udphdr *udp = (struct udphdr *)pkt_data;
            uint16_t src_port = ntohs(udp->source);
            uint16_t dest_port = ntohs(udp->dest);
            if(ip->daddr == inet_addr("127.0.0.1") && ip->protocol == IPPROTO_UDP && dest_port == 5006 && header->len > 0){
                std::cout << "Packet Arrived!!" << std::endl;
                packetSize = header->len;
                char packetArray[packetSize];
                memcpy(packetArray,pkt_data,packetSize);
                
                string packetCpy(packetSize, '\0');
                    
                    for(int i=0;i<packetSize;i++){
                        packetCpy[i] = packetArray[i];
                    }
                    
                    size_t beginning = packetCpy.find("/home");
                    size_t png = packetCpy.find(".png");
                    string mid = packetCpy.substr(beginning);
                    string final=mid.substr(0,png-1);

                    image = cv::imread(final); // OpenCV ile görüntü okuma
                    
                if (!image.empty()) {
                        cout<<"Yol: "<<final<<endl;
                        cout<<packetCount<<". Paket gönderiliyor..."<<endl;
                        handleClient(clientSocket, receiverIP.c_str(), image);
                        fill(packetArray, packetArray + packetSize, 0);
                        packetCount++;
                        packetCpy=empty;
                        packetSize=0;
                        final = empty;
                    } else {
                        std::cerr << "Görüntü okunamadı: " << final << std::endl;
                    } 
            }
        }
    }

    if (res == -1) {
        std::cerr << "Error reading the packets: " << pcap_geterr(fp) << std::endl;
        return -1;
    }
    
    return 0;
}