## On the Desktop
1. Open services and enable the "Smart Card" service. Ensure it is running and set to run on startup
2. Do the same for "Smart Card Emulation Service"
3.
```
net start scardsvr
```
4. Run the Local Group Policy Editor using the gpedit.msc command on the workstation with administrator privileges.
5. Go to Computer configuration → Administrative Templates → Windows Components.
6. Select Remote Desktop Services → Remote Desktop Session Host.
7. Go to Device and Resource Redirection.
8. Select Do not allow supported Plug and Play device redirection.
9. Select Disabled and click ОК.
10. Select Do not allow smart card device redirection.
11. Select Disabled and click ОК.
12. Open the command line as an administrator and run the command:
```
gpupdate /force
```
13. Restart the device.
