/*
 *  tm27.h
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  $Id$
 */

#ifndef _TMTEST27
#error "This is an RTEMS internal file you must not include directly."
#endif

#ifndef __tm27_h
#define __tm27_h

/*
 *  Stuff for Time Test 27
 */

#define MUST_WAIT_FOR_INTERRUPT 1

#define Install_tm27_vector( handler ) \
    set_vector( (handler), 16, 1 )

#define Cause_tm27_intr()  \
    raise( 16 )

#define Clear_tm27_intr()

#define Lower_tm27_intr() \
    _ISR_Set_level( 0 );

#endif
