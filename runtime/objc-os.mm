/*
 * Copyright (c) 2007 Apple Inc.  All Rights Reserved.
 * 
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

/***********************************************************************
* objc-os.m
* OS portability layer.
**********************************************************************/

#include "objc-private.h"
#include "objc-loadmethod.h"

#if TARGET_OS_WIN32

#include "objc-runtime-old.h"
#include "objcrt.h"

const fork_unsafe_lock_t fork_unsafe_lock;

int monitor_init(monitor_t *c) 
{
    // fixme error checking
    HANDLE mutex = CreateMutex(NULL, TRUE, NULL);
    while (!c->mutex) {
        // fixme memory barrier here?
        if (0 == InterlockedCompareExchangePointer(&c->mutex, mutex, 0)) {
            // we win - finish construction
            c->waiters = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
            c->waitersDone = CreateEvent(NULL, FALSE, FALSE, NULL);
            InitializeCriticalSection(&c->waitCountLock);
            c->waitCount = 0;
            c->didBroadcast = 0;
            ReleaseMutex(c->mutex);    
            return 0;
        }
    }

    // someone else allocated the mutex and constructed the monitor
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return 0;
}

void mutex_init(mutex_t *m)
{
    while (!m->lock) {
        CRITICAL_SECTION *newlock = malloc(sizeof(CRITICAL_SECTION));
        InitializeCriticalSection(newlock);
        // fixme memory barrier here?
        if (0 == InterlockedCompareExchangePointer(&m->lock, newlock, 0)) {
            return;
        }
        // someone else installed their lock first
        DeleteCriticalSection(newlock);
        free(newlock);
    }
}


void recursive_mutex_init(recursive_mutex_t *m)
{
    // fixme error checking
    HANDLE newmutex = CreateMutex(NULL, FALSE, NULL);
    while (!m->mutex) {
        // fixme memory barrier here?
        if (0 == InterlockedCompareExchangePointer(&m->mutex, newmutex, 0)) {
            // we win
            return;
        }
    }
    
    // someone else installed their lock first
    CloseHandle(newmutex);
}


WINBOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        environ_init();
        tls_init();
        lock_init();
        sel_init(3500);  // old selector heuristic
        exception_init();
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

OBJC_EXPORT void *_objc_init_image(HMODULE image, const objc_sections *sects)
{
    header_info *hi = malloc(sizeof(header_info));
    size_t count, i;

    hi->mhdr = (const headerType *)image;
    hi->info = sects->iiStart;
    hi->allClassesRealized = NO;
    hi->modules = sects->modStart ? (Module *)((void **)sects->modStart+1) : 0;
    hi->moduleCount = (Module *)sects->modEnd - hi->modules;
    hi->protocols = sects->protoStart ? (struct old_protocol **)((void **)sects->protoStart+1) : 0;
    hi->protocolCount = (struct old_protocol **)sects->protoEnd - hi->protocols;
    hi->imageinfo = NULL;
    hi->imageinfoBytes = 0;
    // hi->imageinfo = sects->iiStart ? (uint8_t *)((void **)sects->iiStart+1) : 0;;
//     hi->imageinfoBytes = (uint8_t *)sects->iiEnd - hi->imageinfo;
    hi->selrefs = sects->selrefsStart ? (SEL *)((void **)sects->selrefsStart+1) : 0;
    hi->selrefCount = (SEL *)sects->selrefsEnd - hi->selrefs;
    hi->clsrefs = sects->clsrefsStart ? (Class *)((void **)sects->clsrefsStart+1) : 0;
    hi->clsrefCount = (Class *)sects->clsrefsEnd - hi->clsrefs;

    count = 0;
    for (i = 0; i < hi->moduleCount; i++) {
        if (hi->modules[i]) count++;
    }
    hi->mod_count = 0;
    hi->mod_ptr = 0;
    if (count > 0) {
        hi->mod_ptr = malloc(count * sizeof(struct objc_module));
        for (i = 0; i < hi->moduleCount; i++) {
            if (hi->modules[i]) memcpy(&hi->mod_ptr[hi->mod_count++], hi->modules[i], sizeof(struct objc_module));
        }
    }
    
    hi->moduleName = malloc(MAX_PATH * sizeof(TCHAR));
    GetModuleFileName((HMODULE)(hi->mhdr), hi->moduleName, MAX_PATH * sizeof(TCHAR));

    appendHeader(hi);

    if (PrintImages) {
        _objc_inform("IMAGES: loading image for %s%s%s%s\n", 
                     hi->fname, 
                     headerIsBundle(hi) ? " (bundle)" : "", 
                     hi->info->isReplacement() ? " (replacement)":"", 
                     hi->info->hasCategoryClassProperties() ? " (has class properties)":"");
    }

    // Count classes. Size various table based on the total.
    int total = 0;
    int unoptimizedTotal = 0;
    {
      if (_getObjc2ClassList(hi, &count)) {
        total += (int)count;
        if (!hi->getInSharedCache()) unoptimizedTotal += count;
      }
    }

    _read_images(&hi, 1, total, unoptimizedTotal);

    return hi;
}

OBJC_EXPORT void _objc_load_image(HMODULE image, header_info *hinfo)
{
    prepare_load_methods(hinfo);
    call_load_methods();
}

OBJC_EXPORT void _objc_unload_image(HMODULE image, header_info *hinfo)
{
    _objc_fatal("image unload not supported");
}


// TARGET_OS_WIN32
#elif TARGET_OS_MAC

#include "objc-file-old.h"
#include "objc-file.h"


/***********************************************************************
* libobjc must never run static destructors. 
* Cover libc's __cxa_atexit with our own definition that runs nothing.
* rdar://21734598  ER: Compiler option to suppress C++ static destructors
**********************************************************************/
extern "C" int __cxa_atexit();
extern "C" int __cxa_atexit() { return 0; }


/***********************************************************************
* bad_magic.
* Return YES if the header has invalid Mach-o magic.
**********************************************************************/
bool bad_magic(const headerType *mhdr)
{
    return (mhdr->magic != MH_MAGIC  &&  mhdr->magic != MH_MAGIC_64  &&  
            mhdr->magic != MH_CIGAM  &&  mhdr->magic != MH_CIGAM_64);
}


static header_info * addHeader(const headerType *mhdr, const char *path, int &totalClasses, int &unoptimizedTotalClasses)
{
    header_info *hi;

    if (bad_magic(mhdr)) return NULL;

    bool inSharedCache = false;

    // Look for hinfo from the dyld shared cache.
    hi = preoptimizedHinfoForHeader(mhdr);
    if (hi) {
        // Found an hinfo in the dyld shared cache.

        // Weed out duplicates.
        if (hi->isLoaded()) {
            return NULL;
        }

        inSharedCache = true;

        // Initialize fields not set by the shared cache
        // hi->next is set by appendHeader
        hi->setLoaded(true);

        if (PrintPreopt) {
            _objc_inform("PREOPTIMIZATION: honoring preoptimized header info at %p for %s", hi, hi->fname());
        }

#if !__OBJC2__
        _objc_fatal("shouldn't be here");
#endif
#if DEBUG
        // Verify image_info
        size_t info_size = 0;
        const objc_image_info *image_info = _getObjcImageInfo(mhdr,&info_size);
        assert(image_info == hi->info());
#endif
    }
    else 
    {
        // Didn't find an hinfo in the dyld shared cache.

        // Weed out duplicates
        for (hi = FirstHeader; hi; hi = hi->getNext()) {
            if (mhdr == hi->mhdr()) return NULL;
        }

        // Locate the __OBJC segment
        size_t info_size = 0;
        unsigned long seg_size;
        const objc_image_info *image_info = _getObjcImageInfo(mhdr,&info_size);
        const uint8_t *objc_segment = getsegmentdata(mhdr,SEG_OBJC,&seg_size);
        if (!objc_segment  &&  !image_info) return NULL;

        // Allocate a header_info entry.
        // Note we also allocate space for a single header_info_rw in the
        // rw_data[] inside header_info.
        hi = (header_info *)calloc(sizeof(header_info) + sizeof(header_info_rw), 1);

        // Set up the new header_info entry.
        hi->setmhdr(mhdr);
#if !__OBJC2__
        // mhdr must already be set
        hi->mod_count = 0;
        hi->mod_ptr = _getObjcModules(hi, &hi->mod_count);
#endif
        // Install a placeholder image_info if absent to simplify code elsewhere
        static const objc_image_info emptyInfo = {0, 0};
        hi->setinfo(image_info ?: &emptyInfo);

        hi->setLoaded(true);
        hi->setAllClassesRealized(NO);
    }

#if __OBJC2__
    {
        size_t count = 0;
        if (_getObjc2ClassList(hi, &count)) {
            totalClasses += (int)count;
            if (!inSharedCache) unoptimizedTotalClasses += count;
        }
    }
#endif

    appendHeader(hi);
    
    return hi;
}


/***********************************************************************
* linksToLibrary
* Returns true if the image links directly to a dylib whose install name 
* is exactly the given name.
**********************************************************************/
bool
linksToLibrary(const header_info *hi, const char *name)
{
    const struct dylib_command *cmd;
    unsigned long i;
    
    cmd = (const struct dylib_command *) (hi->mhdr() + 1);
    for (i = 0; i < hi->mhdr()->ncmds; i++) {
        if (cmd->cmd == LC_LOAD_DYLIB  ||  cmd->cmd == LC_LOAD_UPWARD_DYLIB  ||
            cmd->cmd == LC_LOAD_WEAK_DYLIB  ||  cmd->cmd == LC_REEXPORT_DYLIB)
        {
            const char *dylib = cmd->dylib.name.offset + (const char *)cmd;
            if (0 == strcmp(dylib, name)) return true;
        }
        cmd = (const struct dylib_command *)((char *)cmd + cmd->cmdsize);
    }

    return false;
}


#if SUPPORT_GC_COMPAT

/***********************************************************************
* shouldRejectGCApp
* Return YES if the executable requires GC.
**********************************************************************/
static bool shouldRejectGCApp(const header_info *hi)
{
    assert(hi->mhdr()->filetype == MH_EXECUTE);

    if (!hi->info()->supportsGC()) {
        // App does not use GC. Don't reject it.
        return NO;
    }
        
    // Exception: Trivial AppleScriptObjC apps can run without GC.
    // 1. executable defines no classes
    // 2. executable references NSBundle only
    // 3. executable links to AppleScriptObjC.framework
    // Note that objc_appRequiresGC() also knows about this.
    size_t classcount = 0;
    size_t refcount = 0;
#if __OBJC2__
    _getObjc2ClassList(hi, &classcount);
    _getObjc2ClassRefs(hi, &refcount);
#else
    if (hi->mod_count == 0  ||  (hi->mod_count == 1 && !hi->mod_ptr[0].symtab)) classcount = 0;
    else classcount = 1;
    _getObjcClassRefs(hi, &refcount);
#endif
    if (classcount == 0  &&  refcount == 1  &&  
        linksToLibrary(hi, "/System/Library/Frameworks"
                       "/AppleScriptObjC.framework/Versions/A"
                       "/AppleScriptObjC"))
    {
        // It's AppleScriptObjC. Don't reject it.
        return NO;
    } 
    else {
        // GC and not trivial AppleScriptObjC. Reject it.
        return YES;
    }
}


/***********************************************************************
* rejectGCImage
* Halt if an image requires GC.
* Testing of the main executable should use rejectGCApp() instead.
**********************************************************************/
static bool shouldRejectGCImage(const headerType *mhdr)
{
    assert(mhdr->filetype != MH_EXECUTE);

    objc_image_info *image_info;
    size_t size;
    
#if !__OBJC2__
    unsigned long seg_size;
    // 32-bit: __OBJC seg but no image_info means no GC support
    if (!getsegmentdata(mhdr, "__OBJC", &seg_size)) {
        // Not objc, therefore not GC. Don't reject it.
        return NO;
    }
    image_info = _getObjcImageInfo(mhdr, &size);
    if (!image_info) {
        // No image_info, therefore not GC. Don't reject it.
        return NO;
    }
#else
    // 64-bit: no image_info means no objc at all
    image_info = _getObjcImageInfo(mhdr, &size);
    if (!image_info) {
        // Not objc, therefore not GC. Don't reject it.
        return NO;
    }
#endif

    return image_info->requiresGC();
}

// SUPPORT_GC_COMPAT
#endif


/***********************************************************************
* map_images_nolock
* Process the given images which are being mapped in by dyld.
* All class registration and fixups are performed (or deferred pending
* discovery of missing superclasses etc), and +load methods are called.
*
* info[] is in bottom-up order i.e. libobjc will be earlier in the 
* array than any library that links to libobjc.
*
* Locking: loadMethodLock(old) or runtimeLock(new) acquired by map_images.
**********************************************************************/
#if __OBJC2__
#include "objc-file.h"
#else
#include "objc-file-old.h"
#endif

void 
map_images_nolock(unsigned mhCount, const char * const mhPaths[],
                  const struct mach_header * const mhdrs[])
{
    static bool firstTime = YES;
    header_info *hList[mhCount];
    uint32_t hCount;
    size_t selrefCount = 0;

    // Perform first-time initialization if necessary.
    // This function is called before ordinary library initializers. 
    // fixme defer initialization until an objc-using image is found?
    if (firstTime) {
        preopt_init();
    }

    if (PrintImages) {
        _objc_inform("IMAGES: processing %u newly-mapped images...\n", mhCount);
    }


    // Find all images with Objective-C metadata.
    hCount = 0;

    // Count classes. Size various table based on the total.
    int totalClasses = 0;
    int unoptimizedTotalClasses = 0;
    {
        uint32_t i = mhCount;
        while (i--) {
            const headerType *mhdr = (const headerType *)mhdrs[i];

            auto hi = addHeader(mhdr, mhPaths[i], totalClasses, unoptimizedTotalClasses);
            if (!hi) {
                // no objc data in this entry
                continue;
            }
            
            if (mhdr->filetype == MH_EXECUTE) {
                // Size some data structures based on main executable's size
#if __OBJC2__
                size_t count;
                _getObjc2SelectorRefs(hi, &count);
                selrefCount += count;
                _getObjc2MessageRefs(hi, &count);
                selrefCount += count;
#else
                _getObjcSelectorRefs(hi, &selrefCount);
#endif
                
#if SUPPORT_GC_COMPAT
                // Halt if this is a GC app.
                if (shouldRejectGCApp(hi)) {
                    _objc_fatal_with_reason
                        (OBJC_EXIT_REASON_GC_NOT_SUPPORTED, 
                         OS_REASON_FLAG_CONSISTENT_FAILURE, 
                         "Objective-C garbage collection " 
                         "is no longer supported.");
                }
#endif
            }
            
            hList[hCount++] = hi;
            
            if (PrintImages) {
                _objc_inform("IMAGES: loading image for %s%s%s%s%s\n", 
                             hi->fname(),
                             mhdr->filetype == MH_BUNDLE ? " (bundle)" : "",
                             hi->info()->isReplacement() ? " (replacement)" : "",
                             hi->info()->hasCategoryClassProperties() ? " (has class properties)" : "",
                             hi->info()->optimizedByDyld()?" (preoptimized)":"");
            }
        }
    }

    // Perform one-time runtime initialization that must be deferred until 
    // the executable itself is found. This needs to be done before 
    // further initialization.
    // (The executable may not be present in this infoList if the 
    // executable does not contain Objective-C code but Objective-C 
    // is dynamically loaded later.
    if (firstTime) {
        sel_init(selrefCount);
        arr_init();

#if SUPPORT_GC_COMPAT
        // Reject any GC images linked to the main executable.
        // We already rejected the app itself above.
        // Images loaded after launch will be rejected by dyld.

        for (uint32_t i = 0; i < hCount; i++) {
            auto hi = hList[i];
            auto mh = hi->mhdr();
            if (mh->filetype != MH_EXECUTE  &&  shouldRejectGCImage(mh)) {
                _objc_fatal_with_reason
                    (OBJC_EXIT_REASON_GC_NOT_SUPPORTED, 
                     OS_REASON_FLAG_CONSISTENT_FAILURE, 
                     "%s requires Objective-C garbage collection "
                     "which is no longer supported.", hi->fname());
            }
        }
#endif

#if TARGET_OS_OSX
        // Disable +initialize fork safety if the app is too old (< 10.13).
        // Disable +initialize fork safety if the app has a
        //   __DATA,__objc_fork_ok section.

        if (dyld_get_program_sdk_version() < DYLD_MACOSX_VERSION_10_13) {
            DisableInitializeForkSafety = true;
            if (PrintInitializing) {
                _objc_inform("INITIALIZE: disabling +initialize fork "
                             "safety enforcement because the app is "
                             "too old (SDK version " SDK_FORMAT ")",
                             FORMAT_SDK(dyld_get_program_sdk_version()));
            }
        }

        for (uint32_t i = 0; i < hCount; i++) {
            auto hi = hList[i];
            auto mh = hi->mhdr();
            if (mh->filetype != MH_EXECUTE) continue;
            unsigned long size;
            if (getsectiondata(hi->mhdr(), "__DATA", "__objc_fork_ok", &size)) {
                DisableInitializeForkSafety = true;
                if (PrintInitializing) {
                    _objc_inform("INITIALIZE: disabling +initialize fork "
                                 "safety enforcement because the app has "
                                 "a __DATA,__objc_fork_ok section");
                }
            }
            break;  // assume only one MH_EXECUTE image
        }
#endif

    }

    if (hCount > 0) {
        //读取镜像信息
        _read_images(hList, hCount, totalClasses, unoptimizedTotalClasses);
    }

    firstTime = NO;
}


/***********************************************************************
* unmap_image_nolock
* Process the given image which is about to be unmapped by dyld.
* mh is mach_header instead of headerType because that's what 
*   dyld_priv.h says even for 64-bit.
* 
* Locking: loadMethodLock(both) and runtimeLock(new) acquired by unmap_image.
**********************************************************************/
void 
unmap_image_nolock(const struct mach_header *mh)
{
    if (PrintImages) {
        _objc_inform("IMAGES: processing 1 newly-unmapped image...\n");
    }

    header_info *hi;
    
    // Find the runtime's header_info struct for the image
    for (hi = FirstHeader; hi != NULL; hi = hi->getNext()) {
        if (hi->mhdr() == (const headerType *)mh) {
            break;
        }
    }

    if (!hi) return;

    if (PrintImages) {
        _objc_inform("IMAGES: unloading image for %s%s%s\n", 
                     hi->fname(),
                     hi->mhdr()->filetype == MH_BUNDLE ? " (bundle)" : "",
                     hi->info()->isReplacement() ? " (replacement)" : "");
    }

    _unload_image(hi);

    // Remove header_info from header list
    removeHeader(hi);
    free(hi);
}


/***********************************************************************
* static_init
* Run C++ static constructor functions.
* libc calls _objc_init() before dyld would call our static constructors, 
* so we have to do it ourselves.
**********************************************************************/
static void static_init()
{
    size_t count;
    auto inits = getLibobjcInitializers(&_mh_dylib_header, &count);
    for (size_t i = 0; i < count; i++) {
        inits[i]();
    }
}


/***********************************************************************
* _objc_atfork_prepare
* _objc_atfork_parent
* _objc_atfork_child
* Allow ObjC to be used between fork() and exec().
* libc requires this because it has fork-safe functions that use os_objects.
*
* _objc_atfork_prepare() acquires all locks.
* _objc_atfork_parent() releases the locks again.
* _objc_atfork_child() forcibly resets the locks.
**********************************************************************/

// Declare lock ordering.
#if LOCKDEBUG
__attribute__((constructor))
static void defineLockOrder()
{
    // Every lock precedes crashlog_lock
    // on the assumption that fatal errors could be anywhere.
    lockdebug_lock_precedes_lock(&loadMethodLock, &crashlog_lock);
    lockdebug_lock_precedes_lock(&classInitLock, &crashlog_lock);
#if __OBJC2__
    lockdebug_lock_precedes_lock(&runtimeLock, &crashlog_lock);
    lockdebug_lock_precedes_lock(&DemangleCacheLock, &crashlog_lock);
#else
    lockdebug_lock_precedes_lock(&classLock, &crashlog_lock);
    lockdebug_lock_precedes_lock(&methodListLock, &crashlog_lock);
    lockdebug_lock_precedes_lock(&NXUniqueStringLock, &crashlog_lock);
    lockdebug_lock_precedes_lock(&impLock, &crashlog_lock);
#endif
    lockdebug_lock_precedes_lock(&selLock, &crashlog_lock);
    lockdebug_lock_precedes_lock(&cacheUpdateLock, &crashlog_lock);
    lockdebug_lock_precedes_lock(&objcMsgLogLock, &crashlog_lock);
    lockdebug_lock_precedes_lock(&AltHandlerDebugLock, &crashlog_lock);
    lockdebug_lock_precedes_lock(&AssociationsManagerLock, &crashlog_lock);
    SideTableLocksPrecedeLock(&crashlog_lock);
    PropertyLocks.precedeLock(&crashlog_lock);
    StructLocks.precedeLock(&crashlog_lock);
    CppObjectLocks.precedeLock(&crashlog_lock);

    // loadMethodLock precedes everything
    // because it is held while +load methods run
    lockdebug_lock_precedes_lock(&loadMethodLock, &classInitLock);
#if __OBJC2__
    lockdebug_lock_precedes_lock(&loadMethodLock, &runtimeLock);
    lockdebug_lock_precedes_lock(&loadMethodLock, &DemangleCacheLock);
#else
    lockdebug_lock_precedes_lock(&loadMethodLock, &methodListLock);
    lockdebug_lock_precedes_lock(&loadMethodLock, &classLock);
    lockdebug_lock_precedes_lock(&loadMethodLock, &NXUniqueStringLock);
    lockdebug_lock_precedes_lock(&loadMethodLock, &impLock);
#endif
    lockdebug_lock_precedes_lock(&loadMethodLock, &selLock);
    lockdebug_lock_precedes_lock(&loadMethodLock, &cacheUpdateLock);
    lockdebug_lock_precedes_lock(&loadMethodLock, &objcMsgLogLock);
    lockdebug_lock_precedes_lock(&loadMethodLock, &AltHandlerDebugLock);
    lockdebug_lock_precedes_lock(&loadMethodLock, &AssociationsManagerLock);
    SideTableLocksSucceedLock(&loadMethodLock);
    PropertyLocks.succeedLock(&loadMethodLock);
    StructLocks.succeedLock(&loadMethodLock);
    CppObjectLocks.succeedLock(&loadMethodLock);

    // PropertyLocks and CppObjectLocks and AssociationManagerLock 
    // precede everything because they are held while objc_retain() 
    // or C++ copy are called.
    // (StructLocks do not precede everything because it calls memmove only.)
    auto PropertyAndCppObjectAndAssocLocksPrecedeLock = [&](const void *lock) {
        PropertyLocks.precedeLock(lock);
        CppObjectLocks.precedeLock(lock);
        lockdebug_lock_precedes_lock(&AssociationsManagerLock, lock);
    };
#if __OBJC2__
    PropertyAndCppObjectAndAssocLocksPrecedeLock(&runtimeLock);
    PropertyAndCppObjectAndAssocLocksPrecedeLock(&DemangleCacheLock);
#else
    PropertyAndCppObjectAndAssocLocksPrecedeLock(&methodListLock);
    PropertyAndCppObjectAndAssocLocksPrecedeLock(&classLock);
    PropertyAndCppObjectAndAssocLocksPrecedeLock(&NXUniqueStringLock);
    PropertyAndCppObjectAndAssocLocksPrecedeLock(&impLock);
#endif
    PropertyAndCppObjectAndAssocLocksPrecedeLock(&classInitLock);
    PropertyAndCppObjectAndAssocLocksPrecedeLock(&selLock);
    PropertyAndCppObjectAndAssocLocksPrecedeLock(&cacheUpdateLock);
    PropertyAndCppObjectAndAssocLocksPrecedeLock(&objcMsgLogLock);
    PropertyAndCppObjectAndAssocLocksPrecedeLock(&AltHandlerDebugLock);

    SideTableLocksSucceedLocks(PropertyLocks);
    SideTableLocksSucceedLocks(CppObjectLocks);
    SideTableLocksSucceedLock(&AssociationsManagerLock);

    PropertyLocks.precedeLock(&AssociationsManagerLock);
    CppObjectLocks.precedeLock(&AssociationsManagerLock);
    
#if __OBJC2__
    lockdebug_lock_precedes_lock(&classInitLock, &runtimeLock);
#endif

#if __OBJC2__
    // Runtime operations may occur inside SideTable locks
    // (such as storeWeak calling getMethodImplementation)
    SideTableLocksPrecedeLock(&runtimeLock);
    SideTableLocksPrecedeLock(&classInitLock);
    // Some operations may occur inside runtimeLock.
    lockdebug_lock_precedes_lock(&runtimeLock, &selLock);
    lockdebug_lock_precedes_lock(&runtimeLock, &cacheUpdateLock);
    lockdebug_lock_precedes_lock(&runtimeLock, &DemangleCacheLock);
#else
    // Runtime operations may occur inside SideTable locks
    // (such as storeWeak calling getMethodImplementation)
    SideTableLocksPrecedeLock(&methodListLock);
    SideTableLocksPrecedeLock(&classInitLock);
    // Method lookup and fixup.
    lockdebug_lock_precedes_lock(&methodListLock, &classLock);
    lockdebug_lock_precedes_lock(&methodListLock, &selLock);
    lockdebug_lock_precedes_lock(&methodListLock, &cacheUpdateLock);
    lockdebug_lock_precedes_lock(&methodListLock, &impLock);
    lockdebug_lock_precedes_lock(&classLock, &selLock);
    lockdebug_lock_precedes_lock(&classLock, &cacheUpdateLock);
#endif

    // Striped locks use address order internally.
    SideTableDefineLockOrder();
    PropertyLocks.defineLockOrder();
    StructLocks.defineLockOrder();
    CppObjectLocks.defineLockOrder();
}
// LOCKDEBUG
#endif

static bool ForkIsMultithreaded;
void _objc_atfork_prepare()
{
    // Save threaded-ness for the child's use.
    ForkIsMultithreaded = pthread_is_threaded_np();

    lockdebug_assert_no_locks_locked();
    lockdebug_setInForkPrepare(true);

    loadMethodLock.lock();
    PropertyLocks.lockAll();
    CppObjectLocks.lockAll();
    AssociationsManagerLock.lock();
    SideTableLockAll();
    classInitLock.enter();
#if __OBJC2__
    runtimeLock.lock();
    DemangleCacheLock.lock();
#else
    methodListLock.lock();
    classLock.lock();
    NXUniqueStringLock.lock();
    impLock.lock();
#endif
    selLock.lock();
    cacheUpdateLock.lock();
    objcMsgLogLock.lock();
    AltHandlerDebugLock.lock();
    StructLocks.lockAll();
    crashlog_lock.lock();

    lockdebug_assert_all_locks_locked();
    lockdebug_setInForkPrepare(false);
}

void _objc_atfork_parent()
{
    lockdebug_assert_all_locks_locked();

    CppObjectLocks.unlockAll();
    StructLocks.unlockAll();
    PropertyLocks.unlockAll();
    AssociationsManagerLock.unlock();
    AltHandlerDebugLock.unlock();
    objcMsgLogLock.unlock();
    crashlog_lock.unlock();
    loadMethodLock.unlock();
    cacheUpdateLock.unlock();
    selLock.unlock();
    SideTableUnlockAll();
#if __OBJC2__
    DemangleCacheLock.unlock();
    runtimeLock.unlock();
#else
    impLock.unlock();
    NXUniqueStringLock.unlock();
    methodListLock.unlock();
    classLock.unlock();
#endif
    classInitLock.leave();

    lockdebug_assert_no_locks_locked();
}

void _objc_atfork_child()
{
    // Turn on +initialize fork safety enforcement if applicable.
    if (ForkIsMultithreaded  &&  !DisableInitializeForkSafety) {
        MultithreadedForkChild = true;
    }

    lockdebug_assert_all_locks_locked();

    CppObjectLocks.forceResetAll();
    StructLocks.forceResetAll();
    PropertyLocks.forceResetAll();
    AssociationsManagerLock.forceReset();
    AltHandlerDebugLock.forceReset();
    objcMsgLogLock.forceReset();
    crashlog_lock.forceReset();
    loadMethodLock.forceReset();
    cacheUpdateLock.forceReset();
    selLock.forceReset();
    SideTableForceResetAll();
#if __OBJC2__
    DemangleCacheLock.forceReset();
    runtimeLock.forceReset();
#else
    impLock.forceReset();
    NXUniqueStringLock.forceReset();
    methodListLock.forceReset();
    classLock.forceReset();
#endif
    classInitLock.forceReset();

    lockdebug_assert_no_locks_locked();
}

//iOS 中用到的所有系统 framework 都是动态链接的，类比成插头和插排，静态链接的代码在编译后的静态链接过程就将插头和插排一个个插好，运行时直接执行二进制文件；
//而动态链接需要在程序启动时去完成“插插销”的过程，所以在我们写的代码执行前，动态链接器需要完成准备工作。
//系统使用动态链接有几点好处：
//代码共用：很多程序都动态链接了这些 lib，但它们在内存和磁盘中中只有一份
//易于维护：由于被依赖的 lib 是程序执行时才 link 的，所以这些 lib 很容易做更新，比如libSystem.dylib 是 libSystem.B.dylib 的替身，哪天想升级直接换成 libSystem.C.dylib 然后再替换替身就行了
//减少可执行文件体积：相比静态链接，动态链接在编译时不需要打进去，所以可执行文件的体积要小很多
//dyld（the dynamic link editor），Apple 的动态链接器，系统 kernel 做好启动程序的初始准备后，交给 dyld 负责，
//1. dyld 开始将程序二进制文件初始化
//2. 交由 ImageLoader 读取 image，其中包含了我们的类、方法等各种符号
//3. 由于 runtime 向 dyld 绑定了回调，当 image 加载到内存后，dyld 会通知 runtime 进行处理
//4. runtime 接手后调用 map_images 做解析和处理，接下来 load_images 中调用 call_load_methods 方法，
//5. 遍历所有加载进来的 Class，按继承层级依次调用 Class 的 +load 方法和其 Category 的 +load 方法
//6.至此，可执行文件中和动态库所有的符号（Class，Protocol，Selector，IMP，…）都已经按格式成功加载到内存中，被 runtime 所管理，
//  再这之后，runtime 的那些方法（动态添加 Class、swizzle 等等才能生效）
//重载自己 Class 的 +load 方法时需不需要调父类？runtime 负责按继承顺序递归调用，所以我们不能调 super
//动态链接依赖库，并由 runtime 负责加载成 objc 定义的结构，所有初始化工作结束后，dyld 调用真正的 main 函数。
//当这一切都结束时，dyld 会清理现场，将调用栈回归，只剩下：main()


//kernel准备环境,将应用程序装载到内存中
//kernel装载dyld程序,并调用dyld进行应用初始化操作
//load dylibs 根据Mach-O的Load Commands段加载所有依赖的动态库
//rebase 将经过ASLR随机偏移过的访问首地址创建偏移指针并存储到__DATA段,方便后续访问使用
//bind 将应用程序内调用外部的指令绑定起来
//Objc 调用Objc的runtime环境并初始化,处理category和调用+load()方法
//initializers 调用所有动态库的initializer方法,初始化动态库
//call main() 调用app入口函数main
/***********************************************************************
* _objc_init
* 引导初始化,用Dyld注册我们的镜像通知程序 这里是 objc 和 runtime 的初始化入口
* Bootstrap initialization. Registers our image notifier with dyld.
* Called by libSystem BEFORE library initialization time
**********************************************************************/

void _objc_init(void)
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    
    // fixme defer initialization until an objc-using image is found?
    environ_init();
    tls_init();
    static_init();
    lock_init();
    exception_init();

    //_dyld_objc_notify_register(&map_2_images, load_images, unmap_image);
    //想法dyld注册了回调函数,当imagemap到内存中,当初始化完成image时和卸载image的时候都会回调注册者
    _dyld_objc_notify_register(&map_images, load_images, unmap_image);
}


/***********************************************************************
* _headerForAddress.
* addr can be a class or a category
**********************************************************************/
static const header_info *_headerForAddress(void *addr)
{
#if __OBJC2__
    const char *segnames[] = { "__DATA", "__DATA_CONST", "__DATA_DIRTY" };
#else
    const char *segnames[] = { "__OBJC" };
#endif
    header_info *hi;

    for (hi = FirstHeader; hi != NULL; hi = hi->getNext()) {
        for (size_t i = 0; i < sizeof(segnames)/sizeof(segnames[0]); i++) {
            unsigned long seg_size;            
            uint8_t *seg = getsegmentdata(hi->mhdr(), segnames[i], &seg_size);
            if (!seg) continue;
            
            // Is the class in this header?
            if ((uint8_t *)addr >= seg  &&  (uint8_t *)addr < seg + seg_size) {
                return hi;
            }
        }
    }

    // Not found
    return 0;
}


/***********************************************************************
* _headerForClass
* Return the image header containing this class, or NULL.
* Returns NULL on runtime-constructed classes, and the NSCF classes.
**********************************************************************/
const header_info *_headerForClass(Class cls)
{
    return _headerForAddress(cls);
}


/**********************************************************************
* secure_open
* Securely open a file from a world-writable directory (like /tmp)
* If the file does not exist, it will be atomically created with mode 0600
* If the file exists, it must be, and remain after opening: 
*   1. a regular file (in particular, not a symlink)
*   2. owned by euid
*   3. permissions 0600
*   4. link count == 1
* Returns a file descriptor or -1. Errno may or may not be set on error.
**********************************************************************/
int secure_open(const char *filename, int flags, uid_t euid)
{
    struct stat fs, ls;
    int fd = -1;
    bool truncate = NO;
    bool create = NO;

    if (flags & O_TRUNC) {
        // Don't truncate the file until after it is open and verified.
        truncate = YES;
        flags &= ~O_TRUNC;
    }
    if (flags & O_CREAT) {
        // Don't create except when we're ready for it
        create = YES;
        flags &= ~O_CREAT;
        flags &= ~O_EXCL;
    }

    if (lstat(filename, &ls) < 0) {
        if (errno == ENOENT  &&  create) {
            // No such file - create it
            fd = open(filename, flags | O_CREAT | O_EXCL, 0600);
            if (fd >= 0) {
                // File was created successfully.
                // New file does not need to be truncated.
                return fd;
            } else {
                // File creation failed.
                return -1;
            }
        } else {
            // lstat failed, or user doesn't want to create the file
            return -1;
        }
    } else {
        // lstat succeeded - verify attributes and open
        if (S_ISREG(ls.st_mode)  &&  // regular file?
            ls.st_nlink == 1  &&     // link count == 1?
            ls.st_uid == euid  &&    // owned by euid?
            (ls.st_mode & ALLPERMS) == (S_IRUSR | S_IWUSR))  // mode 0600?
        {
            // Attributes look ok - open it and check attributes again
            fd = open(filename, flags, 0000);
            if (fd >= 0) {
                // File is open - double-check attributes
                if (0 == fstat(fd, &fs)  &&  
                    fs.st_nlink == ls.st_nlink  &&  // link count == 1?
                    fs.st_uid == ls.st_uid  &&      // owned by euid?
                    fs.st_mode == ls.st_mode  &&    // regular file, 0600?
                    fs.st_ino == ls.st_ino  &&      // same inode as before?
                    fs.st_dev == ls.st_dev)         // same device as before?
                {
                    // File is open and OK
                    if (truncate) ftruncate(fd, 0);
                    return fd;
                } else {
                    // Opened file looks funny - close it
                    close(fd);
                    return -1;
                }
            } else {
                // File didn't open
                return -1;
            }
        } else {
            // Unopened file looks funny - don't open it
            return -1;
        }
    }
}


#if TARGET_OS_IPHONE

const char *__crashreporter_info__ = NULL;

const char *CRSetCrashLogMessage(const char *msg)
{
    __crashreporter_info__ = msg;
    return msg;
}
const char *CRGetCrashLogMessage(void)
{
    return __crashreporter_info__;
}

#endif

// TARGET_OS_MAC
#else


#error unknown OS


#endif
