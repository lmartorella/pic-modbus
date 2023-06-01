*** Settings ***
Library    AtKeywords.py    /dev/tty0
Test Setup    Test Setup
Test Teardown    Test Teardown

*** Keywords ***
Test Setup
    Set Default Baud Rate

    # Check AT connection
    Send Command    AT     OK

    # Upgrade Baud rate
    Send Baud Rate Change Command    AT+UART_CUR\=115200,8,1,0,0    OK  baud=115200  data_bits=8   stop_bits=1   parity=none    flow_control=none

    # No echo
    Send Command    ATE0    OK

    # Get module version
    Send Command    AT+GMR    *   *   *   *    OK


Test Teardown
    # Restart the module
    Send Command    AT+RST      OK

Setup WiFi
    # Check that is not connected
    Send Command    AT+CIFSR?   NO

    # Init Wi-Fi
    Send Command    AT+CWINIT\=1     OK
    
    # Don't save data in the non-volatile storage
    Send Command    AT+SYSSTORE\=0   OK

    # Don't want ipv6
    Send Command    AT+CIPV6\=0  OK

    # Set the Wi-Fi mode to station, no auto-connect
    Send Command    AT+CWMODE\=1,0   OK

    # Set the Wi-Fi auto-reconnect mode
    Send Command    AT+CWRECONNCFG\=10,0     OK

*** Test Cases ***
List Available APs
    Setup WiFi

    # List available APs
    Send Command    AT+CWLAP     **

Connect to Wi-Fi
    Setup WiFi

    # Max TX power
    Send Command    AT+RFPOWER\=84  OK

    # Connect to the AP
    Send Command    AT+CWJAP\="%{AT_SSID}",%{AT_PWD}   WIFI CONNECTED    WIFI GOT IP      OK

    # Wait for connection
    Send Command    AT+CWSTATE?     +CWSTATE:2,"%{AT_SSID}"    OK
