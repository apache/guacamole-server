/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"

#include "winpr-stream.h"
#include "winpr-wtypes.h"

wStream* Stream_New(BYTE* buffer, size_t size) {

    /* If no buffer is provided, allocate a new stream of the given size */
    if (buffer == NULL)
       return stream_new(size);

    /* Otherwise allocate an empty stream and assign the given buffer */
    wStream* stream = stream_new(0);
    stream_attach(stream, buffer, size);
    return stream;

}

void Stream_Free(wStream* s, BOOL bFreeBuffer) {

    /* Disassociate buffer if it will be freed externally */
    if (!bFreeBuffer)
        stream_detach(s);

    stream_free(s);

}

