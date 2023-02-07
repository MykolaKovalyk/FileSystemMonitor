# File System Monitor

Application that monitors edit, delete and create operations in a filesystem through a background process and a minifilter driver. Written in C, C++ and C#.

## Brief implementation summary

The project consists of 3 crucial parts:

- Minifilter driver - serves as a primary source of data for the frontend application. Written in C.
- Connection client -  connects to the driver through `FilterConnectCommunicationPort` and serves as an API for C# application. Written in C++.
- Application itself - uses client API, receives data through it, processes the data and saves it to local storage. Works as a background process. Written in C#.


## Requirements

- Visual Studio 2022
- .NET 5.0 Runtime
- MSVC v143 - VS 2022 C++ x64/x86 build tools 14.34.31933
- MSVC v143 - VS 2022 C++ x64/x86 Spectre-mitigated libs 14.34.31933
- Windows 11 SDK (10.0.22621.0)
- Windows Driver Kit

## How to build

- Open the project in Visual Studio 2022
### Ensure your MSVC version is correct:

- In solution explorer, right click at **FSMDriver -> Properties**.
- Make sure **Configuration** is set to **All Configurations**

- Now go to **Configuration Properties -> Advanced -> MSVC Toolset Version** and select available (preferrably **14.34.XXXXX**)

### Configure your test certificate
 - Go to **Configuration Properties -> Driver Signing -> General** and set **Test Certificate** to **\<Create Test Certificate...\>**

 Now you're ready to build. Go to **Build**, and choose **Build Solution**. The application will be in the `Build` folder.

## How to run

### Ensure test-signing is on
- Open your **Command Prompt** <ins>as an Administrator</ins>. Then enter the following command. You will need to restart your system after that: 
```PowerShell
bcdedit /set TESSIGNING ON
```
* (Remember to turn this mode off if you don't use the driver anymore, like this: `bcdedit /set TESSIGNING OFF`)
- If after that, you see `Test Mode` label in the bottom right corner on your Desktop, you're good to proceed.

### Configure and install

- Copy the [`config.json`](config.json) file, and paste it into your `Build` folder, on the same level as an executable `FileSystemMonitor.exe`
- Now in your built application folder, go to `Driver` folder and right-click on the file `FSMDriver.inf`. Then click **Install** (If you're on Windows 11, it will be uder **Show more options** tab)
### Check if the driver is installed.
- Open your **Command Prompt** <ins>as an Administrator</ins>. Then enter the following command:
```PowerShell
sc query FSMDriver
```
- If it doesn't print out something like: `[SC] EnumQueryServicesStatus:OpenService FAILED 1060: The specified service does not exist as an installed service`, then you're all set.


### Run the whole thing
 - To run the driver, Open your **Command Prompt** <ins>as an Administrator</ins>, and enter the following command:
 ```PowerShell
sc start FSMDriver
```
- Now go to your `Build` folder, and run `FileSystemMonitorDriver.exe` <ins>as an Administrator too</ins>. You need it to run as admin because it won't connect to the driver otherwise.
- If all is well, the console will open, the connection will be established and you'll see the logs:

```
Successfully established connection with the driver.
Starting message-receiving loop...
Starting message-handling loop...
Ready.
Saving changes... 
```

It means all parts of the application are up and running and all is well. You can now find logs about the changed files in the `Logs` folder, or view changes in realtime in `record.json`.

Consider starting the app in a task scheduler on startup. This way, you can automate the tedious process of starting the app with a simple startup task rule.