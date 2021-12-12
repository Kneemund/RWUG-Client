## RWUG Client

An application for the Wii U, which sends GamePad data
- using DSU/CemuHook to compatible applications (button, stick and motion data) and/or
- to the [RWUG server](https://github.com/Kneemund/RWUG-Server), which emulates virtual controllers (one for button & stick data, one for motion data).

This means that you can run the client - without any additional software on your PC - for applications that support input and motion data via DSU (mainly emulators like Cemu and yuzu). \
Alternatively, you can combine the client with the server and use the controllers it creates for anything you want, e.g. in any application that supports generic controllers. \
You can also use DSU exclusively for motion data and the controller created by the server for input, if the application you're using does not fully support DSU.

### yuzu
In order to enable full DSU support (as opposed to only motion data), you need to check `Enable UDP controllers` in `Emulation -> Configure... -> Controls -> Advanced`.
