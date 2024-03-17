#!/usr/bin/env python3


import subprocess
from utils import success, error, check_open, get_fin_timeout
import time
import threading
import socket


class Server(threading.Thread):
    def __init__(self, port, condition):
        super().__init__()
        self._port = port
        self._size = 0
        self._condition = condition
        self._running = False

    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        with self._condition:
            try:
                sock.bind(("0.0.0.0", self._port))
            except:
                time.sleep(get_fin_timeout() + 5)
                try:
                    sock.bind(("0.0.0.0", self._port))
                except:

                    self._condition.notify()
                    sock.close()
                    return

            self._running = True
            sock.listen(1)

            self._condition.notify()

        client, _ = sock.accept()
        while True:
            data = client.recv(1)
            if not data:
                break
            self._size += 1

        client.close()
        sock.close()

    @property
    def size(self):
        return self._size

    @property
    def running(self):
        return self._running


def test_default_behaviour():

    condition = threading.Condition()

    with condition:
        server = Server(5000, condition)
        server.start()
        condition.wait()

        if not server.running:
            error("Failed to start the server")
            return

    proc = subprocess.Popen(
        ["./client", "127.0.0.1"], stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    proc.communicate()

    server.join()

    if proc.returncode != 0:
        error("On success the client should exit with 0")
        return

    if server.size != 1000:
        error("By default the client should send 1000 bytes to the server")
        return

    success("No issues found in the default client functionality")


def test_name_resolution():
    condition = threading.Condition()

    with condition:
        server = Server(5000, condition)
        server.start()
        condition.wait()
        if not server.running:
            error("Failed to start the server")
            return

    proc = subprocess.Popen(
        ["./client", "localhost"], stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    proc.communicate()
    server.join()

    if proc.returncode != 0:
        error("On success the client should exit with 0")
        return

    if server.size != 1000:
        error("By default the client should send 1000 bytes to the server")
        return

    success("No issues found in name resolution")


def test_non_default_size():
    condition = threading.Condition()

    with condition:
        server = Server(5000, condition)
        server.start()
        condition.wait()
        if not server.running:
            error("Failed to start the server")
            return
    proc = subprocess.Popen(
        ["./client", "--size", "5000", "127.0.0.1"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    proc.communicate()

    server.join()

    if proc.returncode != 0:
        error("On success the client should exit with 0")
        return

    if server.size != 5000:
        error(
            "The client should send a number of bytes equal to the ones specified by the --size flag"
        )
        return

    success("No issues found with the --size option")


def test_congestion_option():
    condition = threading.Condition()
    with condition:
        server = Server(5000, condition)
        server.start()
        condition.wait()
        if not server.running:
            error("Failed to start the server")
            return
    proc = subprocess.Popen(
        ["./client", "--congestion", "reno", "127.0.0.1"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    proc.communicate()
    server.join()

    if proc.returncode != 0:
        error("On success the client should exit with 0")
        return

    success("No issues found with the --congestion option")


def test_port_option():
    condition = threading.Condition()

    with condition:
        server = Server(5001, condition)
        server.start()
        condition.wait()
        if not server.running:
            error("Failed to start the server")
            return

    proc = subprocess.Popen(
        ["./client", "--port", "5001", "127.0.0.1"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    proc.communicate()
    server.join()

    if proc.returncode != 0:
        error("On success the client should exit with 0")
        return

    if server.size != 1000:
        error(
            "The client should connect to the server on the port given by the --port option"
        )
        return

    success("No issues found with the --port option")


def main():
    print("[+] Testing basic client functionality")
    test_default_behaviour()
    test_name_resolution()
    test_non_default_size()
    test_congestion_option()
    test_port_option()


if __name__ == "__main__":
    main()
