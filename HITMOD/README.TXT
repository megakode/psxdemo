[NOTE: this is the old readme, see CHANGES.TXT for the new stuff.]


                        HITMOD PlayStation MOD Player
  
                             -  Version 1.4 -
    
                            by Silpheed/HITMEN

OK, I finally got around to doing an update to my MOD playing routines, and
quite a big update too :)

WHAT'S NEW:

  - Supports ProTracker/FastTracker compatible MODs of up to 16 channels
  - No longer relies on vertical sync -> now both PAL & NTSC compatible
    (also allows changing of the BPM)
  - Improved sound quality
  - Improved speed
  - Many bugs fixed
  - Waiting time for initialising/loading a MOD is hugely reduced
  - Tool provided to convert from MOD -> HIT (the new format used)
  - Second object file supplied for users of GCC  
  - Other stuff I can't remember...

HOW TO USE:

Just look at the example provided, the user callable functions are listed in
HITMOD.H. Basically just include the header and link with HITMOD.OBJ (or use
HITMOD.O from the GNU directory if you are using GCC).

The example program expects to find a .HIT file at $80100000, use the supplied
tool MODCONV.EXE to convert from MOD to HIT format. (I provided a nice six
channel MOD for you to test it).

Sorry, still no Yaroze support yet, but it will hopefully be ready soon.

PROBLEMS:

MODs that use very short samples (like 32 bytes or so) don't sound very good
when played on the PSX. This seems to be due to the data format used by the
SPU, so unfortunately there isn't really much that can be done to fix this. It
means that some MODs just won't be able to be played on the PS, unless you
don't mind it sounding like shit :)

GREETS: (no order)

BlackBag, Napalm, Vision Thing, Roncler Gang, K-Comm, Acon, Nagra, Duddie,
HeroZero, Danzig, Uxorious, Nik, Chemical, JackRipper, Rip, Tratax, Twin,
Snake, McBain, WildFire, Shroomz, Fennec, Segmond, Kingpin, Mush, Barubary,
Rafu, Dbalster, Respond and everyone else!
        


