import pathlib
import socket
import struct
import threading
import logging

filepath = pathlib.Path("test.trace").open("wb")
logger = logging.getLogger(__name__)


def handle_packet(client_socket: socket.socket, client_address):
    logger.info(f"Connected to client {client_address}")
    while True:
        try:
            length = client_socket.recv(4)
        except:
            continue
        if not length:
            continue
        len_ = struct.unpack("!I", length)[0]
        buf = b""
        while len(buf) < len_:
            try:
                current_buf = client_socket.recv(len_ - len(buf))
                if current_buf:
                    buf += current_buf
                    continue
            except BlockingIOError:
                continue
        print(f"recv trace packet size {len_} -> {len(buf)}")
        filepath.write(buf)


def main():
    trace_packet_server = socket.socket()
    trace_packet_server.bind(
        ("127.0.0.1", 10086)
    )
    trace_packet_server.setblocking(False)
    trace_packet_server.listen()
    while True:
        try:
            client, client_addr = trace_packet_server.accept()
            th = threading.Thread(target=handle_packet, args=(client, client_addr))
            th.start()
        except:
            pass


if __name__ == "__main__":
    main()
