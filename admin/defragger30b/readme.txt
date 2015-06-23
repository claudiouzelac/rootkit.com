defragger v3.0beta for nt/2k/xp + source

free.pages.at/blumetools/ - freeware - blume1975@web.de

Defragger for all reachable FAT/NTFS-partitions (all ClusterSizes) 
under Windows NT(SP4)/2K/XP. Commandline options for automatic 
use. Choose-Drives-Dialog, Drive-Information, Cluster-Viewer, 
file-search, single-file-defrag, configurable blocksize, filesizelimit, 
largefile marks. Uses Windows-Defrag-API, no data corruption possible. 
Free sourcecode.

usage:

  defragger.exe [-d] [-o] [-b] [-x] [-q] [volumeletters]
  default without options: display blank system-partition
  -d defrag   
  -o optimize
  -b both defrag and optimize
  -x all lokal disks, VolLetters are ignored
  -q quit after defraging

examples:

  defragger -d CDE 
    defragment Drives C, D and E
  defragger -b -x 
    defragment and optimize all local disks
  defragger -d -o G [equal to] defragger -b G
    defragment and optimize only G

new in 3.0beta :
      - NTFS-Analyzing by scanning MFT
      - NTFS drives with ClusterSizes other than 4kB are supported
      - commandline options
      - developed with VS6, but compiles under LCC32, MinGW, 
        Dev-C++ too (some small changes needed)
      - save options to "WIN.INI" under "[Defragger 3.0]"
      - added process-priority option; even if set to high, cpu-use
        is very low, so maybe only for analyzing usefull
      - added sinlge-file-defrag available from clusterviewer
      - added win-version checking
      - added export of dirlist to <drive>:\defragger.txt
      - on selecting a file in clusterviewer (after search) 
        show position on volume
      - changed from ntdll-NtFsControlFile-calls to the more common
        DeviceIOControl-function
      - fixed win2k/nt incompatibility
      - minor fixes (f.e. drawing during analyze)
      - more speed (analyzing ntfs, drawing)

todo: - DefragByDirectories ... is not ready
      - keyboard support in clusterviewer
      - make correct freespace in clusterviewer if a file has moved
      - mixed c-runtime and other routines - make consistend
      - bring more structure in the whole code (separate drawing 
        from defragging and so on)
      - make up mind with static, inline and other optimizing stuff

bugs: - defragging sparse files
      - file marks are sometimes not correct
      - defragging of locked files/directories on FAT is not possible
        due to defrag-API of Windows

