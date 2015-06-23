This program is a modified version of winkps from "Playing with Windows /dev/(k)mem".
The article basically shows how to go ring0 with out drivers.
My version should run on windows XP and win2k without modification.

If there is an exception when ran, keep re-executing the program until it lists some 
processes.  Sometimes the callgate isn't always installed, sometimes it doesn't walk 
through the whole eprocess chain, but it does eventually.:) I don't know why this is.

crazylord gives an in-depth explanation of the code. However,it's based on the win2k 
eprocess offsets which are different than xp.  Some kernel variables and routines are 
hardcoded for win2k in his article.

 The only thing hardcoded in winepl.c is the offset to the activeprocesslinks in the 
eprocess structure.  Those offsets for nt,win2k,xp haven't changed yet so it's fine. 
Hopefully, someone can find a memmory scan algorithm to find the activeprocesslink
offset for future changes to the eprocess structure.


~Blacksoulman (blacksoulman@hush.com)