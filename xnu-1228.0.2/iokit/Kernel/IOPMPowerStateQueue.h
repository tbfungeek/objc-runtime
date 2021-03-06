/*
 * Copyright (c) 2001-2002 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
 
#ifndef _IOPMPOWERSTATEQUEUE_H_
#define _IOPMPOWERSTATEQUEUE_H_
 
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOEventSource.h>
#include <IOKit/IOService.h>
extern "C" {
    #include <kern/queue.h>
}

class IOPMPowerStateQueue : public IOEventSource
 {
    OSDeclareDefaultStructors(IOPMPowerStateQueue);

private:
    enum {
        kUnIdle = 0,
        kPMFeatureChange = 1
    };

    // Queue of requested states
    struct PowerChangeEntry 
    {
        void                    *next;
        uint16_t                actionType;
        uint32_t                state;
        IOService               *target;
    };

    void                        *changes;
#ifndef __ppc__
    IOLock                      *tmpLock;
#endif

protected:
    virtual bool checkForWork(void);

public:
    //typedef void (*Action)(IOService *target, unsigned long state);

    virtual bool init(OSObject *owner, Action action = 0);

    // static initialiser
    static IOPMPowerStateQueue *PMPowerStateQueue(OSObject *owner);
         
    // Enqueues an activityTickle request to be executed on the workloop
    virtual bool unIdleOccurred(IOService *, unsigned long);
    
    // Enqueues a feature changed notify request to be executed on the workloop
    virtual bool featureChangeOccurred(uint32_t, IOService *);
 };
 
 #endif /* _IOPMPOWERSTATEQUEUE_H_ */
 
