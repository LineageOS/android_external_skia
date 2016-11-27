/*
 * Copyright 2013, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SkThread_trylock.h"
#include "SkTypes.h"

#include <cutils/log.h>
#include <pthread.h>

SkMutex_trylock::SkMutex_trylock()
{
    int status;

    status = pthread_mutex_init(&mtx, NULL);
    if (status != 0) {
       SkASSERT(0 == status);
    }
}

SkMutex_trylock::~SkMutex_trylock()
{
    int status = pthread_mutex_destroy(&mtx);

    if (status != 0) {
        SkASSERT(0 == status);
    }
}

int SkMutex_trylock::acquire()
{
    return pthread_mutex_trylock(&mtx);
}

void SkMutex_trylock::release()
{
    pthread_mutex_unlock(&mtx);
}
