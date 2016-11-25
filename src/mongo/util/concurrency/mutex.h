// @file mutex.h

/*    Copyright 2009 10gen Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#pragma once

#ifdef _WIN32
#include "mongo/platform/windows_basic.h"
#endif

#include "mongo/util/assert_util.h"
#include "mongo/util/static_observer.h"

namespace mongo {

/** The concept with SimpleMutex is that it is a basic lock/unlock
 *  with no special functionality (such as try and try
 *  timeout).  Thus it can be implemented using OS-specific
 *  facilities in all environments (if desired).  On Windows,
 *  the implementation below is faster than boost mutex.
*/

/** Robomongo: 
 *  "_destroyed" flag is added to fix insidious dead reference 
 *  problem where global SimpleMutex object "sslManagerMtx" was being 
 *  locked after it has been destroyed during program exit. As a result 
 *  closing Robomongo results with crash when at least one SSL-enabled 
 *  replica set connection is used. 
*/
    
#if defined(_WIN32)

class SimpleMutex {
    MONGO_DISALLOW_COPYING(SimpleMutex);

public:
    SimpleMutex() {
        InitializeCriticalSection(&_cs);
    }

    ~SimpleMutex() {
        if (!StaticObserver::_destroyingStatics) {
            DeleteCriticalSection(&_cs);
        }
        _destroyed = true;
    }

    void lock() {
        if (!_destroyed)
            EnterCriticalSection(&_cs);
    }
    void unlock() {
        if (!_destroyed)
            LeaveCriticalSection(&_cs);
    }

private:
    CRITICAL_SECTION _cs;
    bool _destroyed = false;
};

#else

class SimpleMutex {
    MONGO_DISALLOW_COPYING(SimpleMutex);

public:
    SimpleMutex() {
        verify(pthread_mutex_init(&_lock, 0) == 0);
    }

    ~SimpleMutex() {
        if (!StaticObserver::_destroyingStatics) {
            verify(pthread_mutex_destroy(&_lock) == 0);
        }
    }

    void lock() {
        if (!_destroyed)
            verify(pthread_mutex_lock(&_lock) == 0);
    }

    void unlock() {
        if (!_destroyed)
            verify(pthread_mutex_unlock(&_lock) == 0);
    }

private:
    pthread_mutex_t _lock;
    bool _destroyed = false;
};
#endif

}  // namespace mongo
