Keyboard Device Logger


:[INTRODUCTION]:

      This was my first attempt to program a device driver and a simple key logger seemed about as easy as any place to start. Device driver programming documentation is fairly scarce in comparison to other projects and most of the code examples are very artificial for someone who does not yet have a grasp on driver program flow. The code itself for the most part is the documentation for the project. I have a hard time keeping track of my thoughts so I talk to myself a lot in the code so I can remember what my reasoning was at that time. That being said I would not doubt it a bit if I have more lines of comments then actual lines of code. I am also a self taught programmer so I am sure a lot of line and variable alignments will seem odd, as well as program flow. For me personally programming is an art form along the lines of painting or sculpting, it is something I do for the creative factor. Program flow is dictated by passion of the process and what is attempting to be accomplished. This driver is not essentially set up as a tutorial but more of a functioning example with semi documented inner workings. It makes the assumption you have the basic concepts of a WDM driver under your belt.


:[OVERVIEW]:

      KDL is a filter driver that attaches itself to the kbdclass and gleams data as it is passed up and down the device stack storing it in a nonpaged memory cache. When the max cache size condition is met the cached key data is processed, formatted, and dumped to file. The rest of the driver is a standard WDM filter driver in its flow and execution with the addition of code that allows for the logging process of key data.

      The loader is HEAVILY based on Ctrl2Cap (www.sysinternals.com) and just rams the registry settings needed into the system. This worked well for the driver since it was the point and the loader is just an end to those means.

This driver in no way tries to hide or disguise it self, but on the contrary "hides" in plain sight.
There are no tricks or any raw manipulations of bits that lock this driver into certain hardware, revision or patch level using WDM standards in an attempt to bend the rules rather then break them. The DDK was used to compile the driver under the W2k/Xp/2003 build environments but only given tested under Xp Sp2. 

      The thought process behind this driver was to remain at a high level of compatibility across patches/service packs by adhering to DDK standards in the strictest interpretation.

      The code itself is compartmentalized into files that best describe what that portion of codes purpose in life is.
      This should help offset the drivers (re)developement cycle over time against patches and new OS versions and for overall general code evolution.


:[BASIC FEATURES]:

Source Level filter function that can be used to customize the type of key data that is logged.

Registry configurable key data cache size. Used to ease I/O overhead. 

Registry configurable log file.

Source level configurable key data output format.


:[FILES]:

The supplied kdl.sys driver in the \bin directory was compiled under the XP Free Build Environment.
Kdl.exe is a command line loader that will provide its own options.
The insatll.bat and uninstall.bat are there because editing the driver can produce lets say "interesting" results and a simple mouse click is all you will be able to pull off to uninstall the driver without other boot means with regedit.


:[UNKNOWNS]:

Kdl has not been tested with USB keyboards and have no idea of how it will react to PnP calls of insertion and removal. Most likely it will have a very nasty reaction. Not having a USB keyboard lying around at the moment this portion of the code is not planed to be implemented anytime soon.


:[POSSIBLE FUTURE INCLUSIONS]:

Command line option for log file attributes

Implement IoInitializeRemoveLock for dynamic loading and unloading

Customizable key filters from user level
