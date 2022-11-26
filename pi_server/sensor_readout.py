import time
import socket
import pickle
import struct
from hdc1080_driver import SDL_Pi_HDC1080


def binary_print_16(byte_data):
    return bin(byte_data)[2:].zfill(16)


def mean(measurements: list) -> tuple:
    mean_temp = 0.0
    mean_humi = 0.0
    for temp, humi in measurements:
        mean_temp += temp
        mean_humi += humi

    return (mean_temp / len(measurements), mean_humi / len(measurements))


class THSensor:
    def __init__(
        self,
        location_label,
        dual_read_mode: bool = True,
        heater_on: bool = False,
    ):
        self.__temperature_label = location_label + ".temperature"
        self.__humidity_label = location_label + ".humidity"

        self.__hdc1080 = SDL_Pi_HDC1080(
            dual_read_mode=dual_read_mode,
            heater_on=heater_on,
            # humidity_converter=lambda x: (x - 6500)
            # / 65536.0
            # * 100.0,  # 4587, 11141 is offset measured at 75.5%
        )
        time.sleep(0.015)

    def read(self) -> tuple:
        """Read temperature and humidity."""

        temp, humi = self.__hdc1080.read_temperature_and_humidity()
        time.sleep(0.1)
        # temp = self.__hdc1080.read_temperature()
        # humi = self.__hdc1080.read_humidity()
        print("temp", temp, "humi", humi)
        return (temp, humi)

    def read_metric_tuple(self):
        timestamp = int(time.time())
        temperature, humidity = mean([self.read() for _ in range(0, 10)])

        return [
            (self.__temperature_label, (timestamp, temperature)),
            (self.__humidity_label, (timestamp, humidity)),
        ]

    def read_config_register(self):
        """Read config register."""
        return binary_print_16(self.__hdc1080.read_config_register())


class DataFeeder:
    def __init__(
        self,
        address: str = "localhost",
        port: int = 2004,
        timeout_in_seconds=10,
    ):
        self.__socket = socket.socket()
        self.__timeout_in_seconds = timeout_in_seconds
        self.connect((address, port))

    def connect(self, address: tuple) -> None:
        """Make a TCP connection to the graphite server."""
        self.__socket.settimeout(self.__timeout_in_seconds)
        try:
            self.__socket.connect(address)
        except socket.timeout:
            raise TimeoutError(
                "Took over %d second(s) to connect to %s"
                % (self.__timeout_in_seconds, address)
            )
        except socket.gaierror:
            raise ConnectionError(
                "No address associated with hostname %s" % address
            )
        except Exception as error:
            raise ConnectionError(
                "unknown exception while connecting to %s - %s"
                % (address, error)
            )

    def upload(self, metrics_tuple: tuple) -> None:
        payload = pickle.dumps(metrics_tuple, protocol=2)
        header = struct.pack("!L", len(payload))
        message = header + payload
        self.__socket.sendall(message)


def run_measurements():
    sensor = THSensor("kitchen")
    print("config register:", sensor.read_config_register())
    data_feeder = DataFeeder(address="localhost", port=2004)

    while True:
        metrics = sensor.read_metric_tuple()
        print(metrics)
        # data_feeder.upload(metrics)
        time.sleep(120)


run_measurements()
