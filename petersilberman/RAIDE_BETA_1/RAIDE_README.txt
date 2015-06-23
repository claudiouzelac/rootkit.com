READ ME: RAIDE

1.	Disclaimer 
2.	Supported Systems
3.	Usage
4.	Bug Reports
5.  Contact
6.	Updates
7.  Version

1.	Disclaimer
By using RAIDE you the user agree that those responsible for developing and distributing this program, including the lead developer, Peter Silberman, are not responsible for any damage, disruptions, or losses of injuries of any kind that RAIDE may cause including without limitation damage to your hard drive, processor or other hardware, damage to your operating system or other software, and losses of data to you or your business. You the user understand and agree that RAIDE is provided AS IS and no representations, warranties or promises of any kind are made with respect to the stability, safety, or effectiveness of this program whether used in its intended fashion or otherwise. By using RAIDE you agree to all of the terms and conditions set forth above.

2. Supported Systems
RAIDE supports XP and XP SP2 as well as Windows 2003 pre SP1. RAIDE will not run on post SP 1 windows 2003. RAIDE has not been fully tested on Windows 2000. I encourage you to send me reports if RAIDE crashes, let me know and tell me how it crashed and how to reproduce the crash. 

3.	Usage
RAIDE has the following scan modes:
scan_processes – scan the system for processes only
scan_hooks – scan the system for hooks only includes both kernel and userland scans
scan_kernel – scan the kernel only for hooks do not scan userland
scan_all – scan userland kernel and for processes
scan_user – scan just the userland
scan – the default scan, best one to use scans for processes and checks the kernel for hooks

If you want to log the output of RAIDE to a file pass it the log option after the scan mode.

4. Bug Reports
As with any kernel drivers it is inevitable that there will be bugs. Please send all bug reports to peter.silberman@gmail.com I will do my best to fix them. I encourage everyone to try and bypass RAIDE its important to elevate the bar with regards to detection and bypassing it. 

5. Contact
Contact me at peter.silberman@gmail.com

6. Updates
To get updates check rootkit.com or openrce.org

7. Version
RAIDE Beta 1.0 - Initial release 8/6/06
