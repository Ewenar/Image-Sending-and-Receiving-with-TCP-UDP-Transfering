import socket

def send_path_via_socket(file_path, host, port):
    try:
        # UDP socket bağlantısı oluştur
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            # Dosya yolunu gönder
            s.sendto(file_path.encode('utf-8'), (host, port))
            print(f"Dosya yolu {host}:{port} adresine UDP ile gönderildi.")
    except Exception as e:
        print(f"Bir hata oluştu: {e}")

if __name__ == "__main__":
    # Kullanıcıdan dosya yolunu al
    file_path = input("Dosya yolunu girin: ")

    host = '127.0.0.1'  # Hedef adres
    port = 5006  # Hedef port (UDP)

    send_path_via_socket(file_path, host, port)

