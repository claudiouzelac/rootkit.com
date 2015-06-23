/*******************************************************************************
 *
 * vrdtsc.c - Timing attack to detect presents of hardware assisted VMM
 *
 * DISCLAIMER: Full license agreement: http://www.bugcheck.org/license.txt
 
 *             The following contents are for educational purposes only. The 
 *             author is not responsible for how they are used or interpreted.
 *
 *
 * Credits go to optyx for discussing this with me originally several weeks ago, 
 * to rich for bringing up the idea as well, and to the rest of the people how 
 * have mentioned it on varies message boards (invisiblethings.org blog comments
 * and openrce.org message thread from what I have seen so far).
 * 
 * 08/13/2006, chris@bugcheck.org
 *
 ******************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////
// GLOBAL DEFINITIONS
//

#define NUM_ITERS (10*1000*1000)
#define NUM_MULTIPLY (100)

////////////////////////////////////////////////////////////////////////////////
// COMPILER AND PROCESSOR SPECIFIC STUFF
//

#ifdef _MSC_VER

void
__declspec(naked)
__fastcall
do_bunch_of_cpuid( int iters ) {
    
    __asm pusha
    __asm mov esi,ecx
    
    do_loop:  
    __asm xor eax,eax  
    __asm cpuid
    __asm dec esi
    __asm cmp esi,0
    __asm jnz do_loop
    
    __asm popa
    __asm ret
}

__int64
__declspec(naked)
__fastcall
do_rdtsc() {
    
    __asm rdtsc
    __asm ret
}

__int64
__declspec(naked)
__fastcall
do_bunch_of_rdtsc( int iters ) {
    
    do_loop:     
    __asm rdtsc
    __asm loopnz do_loop
    __asm ret
}
    
#elif __GNUC__

#error GNUC inline assembler not done yet

#else

#error unknown compiler for inline assembler

#endif    

////////////////////////////////////////////////////////////////////////////////
// STDLIB C CODE FOR TESTING VMEXIT ROUND TRIPS AND RDTSC TIMES ETC
//

void
print_banner( char *program, FILE *fd )
{
    fprintf( fd,
            "**********************************************************\n"
            " VRDTSC - Timing attack to detect hardware assisted VM \n"
            "          The following should NEVER EVER EVER take more then\n"
            "          1 minute to execute, if it does something is wrong!\n"
            "\n"
            " Author: bugcheck\n"
            " Built on: %s\n"
            "**********************************************************\n\n",
            __DATE__
          );
}

//
// returns average execution tick count of the loop in do_bunch_of_cpuid()
//
__int64
get_cpuid_loop_ticks( int iters ) {

    __int64 start,finish;

    start = do_rdtsc();
    do_bunch_of_cpuid(iters);
    finish = do_rdtsc();

    return finish-start;
}

//
// just runs for a long time doing integer multiplication for ~10seconds 
// and returns the before/after of rdtsc and time()
//
void
get_approx_tick_res( __int64 *ticks, time_t *seconds ) {
    
    __int64 sticks,fticks;
    time_t  start,finish;
    
    __int64 i = (1000000000/3*2);
    __int64 j = 37;
    
    start   = time(NULL);
    sticks  = do_rdtsc();
    while(i--) j *= j;
    fticks  = do_rdtsc();
    finish  = time(NULL); 
    
    *ticks  = (fticks-sticks);
    *seconds= (finish-start);
    return;
}

//
// just runs for a long time doing rdtsc in a loop for the specified number
// of iterations, using a multiplier for really large values and returns
// a before/after of rdtsc and time()
//
void
get_rdtsc_exec_info( 
    int iters, int multiplier, __int64 *ticks, time_t *seconds ) {
    
    __int64 sticks,fticks;
    time_t  start,finish;

    start   = time(NULL);
    sticks  = do_rdtsc();
    while(multiplier--) do_bunch_of_rdtsc(iters);
    fticks  = do_rdtsc();
    finish  = time(NULL); 

    *ticks  = (fticks-sticks);
    *seconds= (finish-start);
    return;
}


int
__cdecl
main(

    int argc,
    char *argv[] 
)
{
   int      ret_val; 
    __int64 ticks,tickres,tickres2;
    __int64 total_iters;
    time_t  seconds;

    ret_val = 0;
            
    if( argc != 1 )
    {   
        print_banner( argv[0], stderr );
        fprintf( stderr, 
                "Usage: %s \n"
                "       Dont forget to use your stopwatch too! =P\n", argv[0] );
        exit(-1);
    }
    
    print_banner( argv[0], stdout );

    //
    // VMEXIT estimated roundtrip time on first gen. Intel VT is ~8000ticks and,
    // ~5000 on second gen. ie. Xeon Woodcrest cores. AMDV transition stats are 
    // unknown since I dont have hardware readily available to test on at time 
    // of writing. The CPUID loop above using eax=0 should be ~50ticks in a 
    // native CPU context.
    //
    printf( "Attempting to detect a #VMEXIT on a cpuid instruction...\n" );
    
    ticks = get_cpuid_loop_ticks(NUM_ITERS);
    
    printf( "Total iterations   : %u \n"
            "Total ticks        : 0x%010I64x\n"
            "Ticks per iteration: %I64u\n",
            NUM_ITERS,
            ticks,
            ticks/NUM_ITERS );

    if( ticks/NUM_ITERS < 150 ) {
        printf( "Doesnt look like a VM based on CPUID time to execute\n" );
    } else {        
        printf( "Looks like a VM and CPUID is causing a #VMEXIT\n" );
        ret_val = 0;
    }

    //
    // WOOYAH!!!(NOTE: 32bit mode VT only tested) rdtsc is crazy in virtual 
    // guest mode at over 200 ticks w/o its intercept set. If its intercept 
    // is set it will introduce a full exit to the hypervisor and be as 
    // bad as the above. In native mode an rdtsc with a loopnz together are
    // well under 10 ticks.
    // 
    // reason is something like: 
    //
    //  if(not virtual_mode)
    //    mov edx:eax,$tsc
    //  else
    //   mov edx:eax,$tsc 
    //   add edx:eax, qword ptr [$current_vmcontrol+TSC_OFFSET]
    //
    //
    // With the following, if something funny was happening with the return
    // value of time() the hypervisor may have been fooling around with the
    // delivery of hardware timer interrupts but we'll try and catch that later
    //
    
                
    printf( "\nAttempting to detect VM guest mode "
                     "via rdtsc's TSC_OFFSET overhead...\n" );
    
    get_rdtsc_exec_info(NUM_ITERS, NUM_MULTIPLY, &ticks, &seconds);
    
    total_iters = (__int64)NUM_ITERS*NUM_MULTIPLY;
    tickres = (__int64)((seconds==0) ? 0 : (ticks/seconds));
    
    printf( "Total iterations   : %I64u\n"
            "Total ticks        : 0x%010I64x\n"
            "Ticks per iteration: %I64u\n"
            "Approximate time   : %u seconds\n"
            "Approx. tick res.  : 0x%010I64x / second\n",
            total_iters,
            ticks,
            ticks/total_iters,
            seconds,
            tickres );

    if( seconds < 10 ) {
        printf( "Does not appear to be in a VM based on completion time\n" );
    } else {
        printf( "Appears to be in a VM based on completion time\n" );
        ret_val = 1;
    }
        
    if( ticks/total_iters < 15 ) {
        printf( "Does not appear to a VM based on RDTSC execution time\n" );
    } else {
        printf( "Appears to be in a VM based on RDTSC time to execute\n" );
        ret_val = 1;
    }
                
    //
    // OK so if everything has looked ok so far and somehow a malicious 
    // hypervisor has fooled us (programmability anyway) up to here by 
    // manipulating TCP_OFFSET and the delivery of hardware interrupts or
    // the path from time() to the kernel we might be able to catch it here
    // by doing a less suspicious guesstimate/comparison of the tick resolution.
    // NOTE: Here the resolution can be pretty low since if we have been fooled 
    // this far the resolution skew will be HUGE! Also by this point it would be
    // clearly identified by a human, no timing device required, batteries sold
    // separately.
    //
    
    printf( "\nAttempting to calculate an estimated *kosher* "
            "tick resolution...\n" );
       
    get_approx_tick_res( &ticks, &seconds );
    
    tickres2 = (__int64)((seconds==0) ? 0 : (ticks/seconds));
    
    printf( "Observed 0x%010I64x ticks in %u seconds\n"
            "Estimating approx. 0x%010I64x / second\n", 
            ticks, seconds,
            tickres2 );

    if( tickres == 0 || tickres2 == 0 || tickres == tickres2 ) {
        
        printf( "Could not compare tick resolutions\n" );
        
    } else {
        
        if( max(tickres,tickres2) / min(tickres,tickres2)  > 2 ) {
            printf( "Reported tickcounts severely skewed, "
                    "malicious hypervisor is probably installed!!!\n" );
        
        } else {
            printf( "Reported tick resolutions appear to match, "
                    "no malicious hypervisor suspected.\n" );
            ret_val = 1;
        }
    }

    return;
}
