# OSC-ESP32-Go-Button
ESP32c3 for sending OSC Messages over Wifi with Web Server

This is probably the cheapest wireless Go/Panic button you can build for under €10, made entirely from off-the-shelf components. I appreciate your feedback on this proof of concept.


Features:

Stores credentials for multiple Wi-Fi networks (e.g. Home, Work) along with their corresponding OSC target IPs.

Hosts a built-in web interface to configure the destination IP, port, and custom OSC messages for each button.

Defaults are set for QLab’s Machine, 53000, /go and /panic.


How it works:

On connection, it sends the passcode to whichever QLab workspace is currently active:
/connect STRING(passcode)

Since permissions normally expire after ~61 seconds, it also sends:
/forgetMeNot BOOL(TRUE) to keep the access granted.
(Alternatively, you can send a dummy message “(e.g. /thump)” every <60s to keep the session alive.)

If you open the web interface and update any settings, the device will resend the passcode automatically.
