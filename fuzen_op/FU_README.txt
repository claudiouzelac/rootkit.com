Program:    fu.exe and msdirectx.sys
Written by: fuzen_op
Email:      fuzen_op@yahoo.com or fuzen_op@rootkit.com

Description: 
	fu.exe and msdirectx.sys work as one. fu.exe passes down parameters as IOCTL's
	to the msdirectx.sys driver. As such, once the driver is loaded, you do not need any 
	special privilege to run fu.exe. msdirectx.sys is the driver and does all the work 
	of fu.exe. The driver is never unloaded until reboot. You can use whatever methods 
        you like to load the driver such as SystemLoadAndCallImage suggested by Greg Hoglund. 
        The driver is named msdirectx.sys. It is a play on Microsoft's DirectX and is named
        this to help hide it. (A can use FU to hide it completely!) 

	The FU rootkit can now hide any named driver in a manner similar to the way it hides
	processes. All the code to do this is in the driver (msdirectx.sys). If you want to 
	send IOCTL's to a driver, you need a handle to it. FU makes no effort to hide or delete 
	the symbolic link used to open a handle to the driver to be hidden. You could add this 
	code easily though if you wanted. The msdirectx.sys driver should just delete the symbolic 
	link while it is hiding the driver.


	The driver has many uses. It can change the groups on any process. So,
	you could give your process System by typing:
		fu -pss #process_pid System

	It can also hide a process. Type:
		fu -ph #process_pid

	At times you may want to "adjust" the privileges on a particular process. You can do
	this by typing something like:
		fu -prs #process_pid SeDebugPrivilege
	You will need to type the specific privileges you want, but no worries I have listed
	them in ListPrivileges.txt.  

	Another feature is msdirectx.sys can change the AUTH_ID on any process. This can be used 
	to impersonate another logon session so that Windows Auditing etc. does not know what 
	user really performed the actions you choose to take with the process. Type:
		fu -pas #process_pid
	The process specified now looks like System in the Event Viewer, etc. You can recompile 
	it to use Anonymous_Logon, LocalService, or NetworkService instead of System. See 
	Rootkit.h.

	The driver does all this by Direct Kernel Object Manipulation (TM)!! No worries about do I have 
        permission to that process, token, etc. If you can load a driver once, you are golden! 
	Also, it does not use "hooking" techniques. Hooking is easily detectable (See VICE). FU is much 
	better. It just writes directly to memory because it understands the structures inside
	and out.

Program Usage: 
   fu
        [-pl]  #number   to list the first #number of processes
        [-ph]  #PID      to hide the process with #PID
        [-pld]           to list the named drivers in DbgView
        [-phd] DRIVER_NAME to hide the named driver
        [-pas] #PID      to set the AUTH_ID to SYSTEM on process #PID.
				 Use this to impersonate other people when you
				 do things. 
				 Note: You can recompile it to use Anonymous_Logon, 
				       LocalService, or NetworkService instead of
					 System. See Rootkit.h.
        [-prl]           to list the available privileges
        [-prs] #PID #privilege_name to set privileges on process #PID
        [-pss] #PID #account_name to add #account_name SID to process #PID token



Caveat:
	The binaries I have included will only run on Windows 2000/XP. See above. You
	will definitely have to recompile for NT because the kernel in 2000/XP exports 
	except_handler3 and NT does not so the driver is not compatible across all three.

	WE ARE MODIFYING KERNEL STRUCTURES (OBJECTS) DIRECTLY IN MEMORY. AS SUCH, AT TIMES
	IT CAN CAUSE A BLUESCREEN. I HAVE SEEN IT HAPPEN, BUT I WOULD SAY IT IS 99% TO 99.5%
	STABLE. IT ALL DEPENDS ON WHAT YOU ARE DOING AT THE TIME. 

	IF YOU FIND A PROBLEM OR A BUG, PLEASE EMAIL ME AT THE ABOVE ADDRESS. PROVIDE
	AS MUCH DETAIL AS POSSIBLE ABOUT THE SEQUENCE OF EVENTS. WE MAY ALSO ARRANGE TO SEND 
	ME YOUR PHYSICAL DUMP OF Kernel MEMORY, BUT DON'T SEND THAT IMMEDIATELY AS MY EMAIL WILL NOT
	HOLD IT.

Thanks:
	HexQueenSVH  - Constantly pushes me to do better. Pointed out problems originally with 
		       the token manipulation and a bug that prevented the last process from being
		       displayed.
	Kimmo Kasslin and Opc0de
		     - Helped find a way on XP and 2003 to get PsLoadedModuleList. FU does not use 
		       their method because it did not work on Windows 2000. I found a more generic 
		       solution, but they their efforts still helped motivate me.
	Kimmo Kasslin
		     - Contributed the Windows 2003 offsets.
	Gentleman from the Amsterdam class
		     - Contributed the Windows XP Service Pack 2 Beta offsets	
	Greg Hoglund - the father of Windows rootkits and a code guru.
	Joe          - who made NDIS cry.
	Contagion    - if its on the wire, he knows what to do.

