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

#ifndef GUAC_COMMON_SSH_DSA_COMPAT_H
#define GUAC_COMMON_SSH_DSA_COMPAT_H

#include "config.h"

#include <openssl/bn.h>
#include <openssl/dsa.h>

#ifndef HAVE_DSA_GET0_PQG
/**
 * DSA_get0_pqg() implementation for versions of OpenSSL which lack this
 * function (pre 1.1).
 *
 * See: https://www.openssl.org/docs/man1.1.0/crypto/DSA_get0_pqg.html
 */
void DSA_get0_pqg(const DSA* dsa_key, const BIGNUM** p,
        const BIGNUM** q, const BIGNUM** g);
#endif

#ifndef HAVE_DSA_GET0_KEY
/**
 * DSA_get0_key() implementation for versions of OpenSSL which lack this
 * function (pre 1.1).
 *
 * See: https://www.openssl.org/docs/man1.1.0/crypto/DSA_get0_key.html
 */
void DSA_get0_key(const DSA* dsa_key, const BIGNUM** pub_key,
        const BIGNUM** priv_key);
#endif

#ifndef HAVE_DSA_SIG_GET0
/**
 * DSA_SIG_get0() implementation for versions of OpenSSL which lack this
 * function (pre 1.1).
 *
 * See: https://www.openssl.org/docs/man1.1.0/crypto/DSA_SIG_get0.html
 */
void DSA_SIG_get0(const DSA_SIG* dsa_sig, const BIGNUM** r, const BIGNUM** s);
#endif

#endif

