
*** Test Cases ***
List Available APs
    # Init Wi-Fi
    Send Command    AT+CWINIT\=1     OK
    
    # Don't save data in the non-volatile storage
    Send Command    AT+SYSSTORE\=0   OK

    # Don't want ipv6
    Send Command    AT+CIPV6\=0  OK

    # Set the Wi-Fi mode to station, no auto-connect
    Send Command    AT+CWMODE\=1,0   OK

    # List available APs
    Send Command    AT+CWLAP     *


Connect to Wi-Fi
    # Init Wi-Fi
    Send Command    AT+CWINIT\=1     OK
    
    # Don't save data in the non-volatile storage
    Send Command    AT+SYSSTORE\=0   OK

    # Don't want ipv6
    Send Command    AT+CIPV6\=0  OK

    # Set the Wi-Fi mode to station, auto-connect
    Send Command    AT+CWMODE\=1,1   OK

    # Set the Wi-Fi auto-reconnect mode
    Send Command    AT+CWRECONNCFG\=10,0     OK

    # Connect to the AP
    Send Command    AT+CWJAP\="${ssid}",${pwd}   WIFI CONNECTED  WIFI GOT IP

    # Wait for connection
    Send Command    AT+CWSTATE?     +CWSTATE:2,"${ssid}"
