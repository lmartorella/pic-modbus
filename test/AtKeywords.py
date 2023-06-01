from robot.api.deco import library, keyword
import serial

DEFAULT_PORT_SETTINGS = {
    "baud": 9600,
    "data_bits": 8,
    "stop_bits": 1,
    "parity": "none",
    "flow_control": "none",
}

@library(scope='GLOBAL')
class AtKeywords:
    def __init__(self, device):
        self.ser = serial.Serial(device, timeout=5)
        self.set_default_baud_rate()

    def _set_settings(self, **kwargs):
        pass

    @keyword
    def set_default_baud_rate(self):
        self._set_settings(**DEFAULT_PORT_SETTINGS)

    def _send_command(self, command):
        self.ser.write(command)

    def _wait_response(self, *expected_response):
        while expected_response.count > 0:
            line = self.ser.readline().decode("utf-8").strip()
            if line == "":
                continue
            expected_line = expected_response.pop(0)
            if line != expected_line:
                raise Exception(f"Expected {expected_line} but received {line}")

    @keyword
    def send_command(self, command, *expected_response):
        self._send_command(command)
        self._wait_response(*expected_response)

    @keyword
    def send_baud_rate_change_command(self, command, *expected_response, **kwargs):
        self._send_command(command)
        self._set_settings(**kwargs)
        self._wait_response(*expected_response)

