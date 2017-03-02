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

#include <openssl/bn.h>
#include <openssl/dsa.h>

#include <stdlib.h>

#ifndef HAVE_DSA_GET0_PQG
void DSA_get0_pqg(const DSA* dsa_key, const BIGNUM** p,
        const BIGNUM** q, const BIGNUM** g) {

    /* Retrieve all requested internal values */
    if (p != NULL) *p = dsa_key->p;
    if (q != NULL) *q = dsa_key->q;
    if (g != NULL) *g = dsa_key->g;

}
#endif

#ifndef HAVE_DSA_GET0_KEY
void DSA_get0_key(const DSA* dsa_key, const BIGNUM** pub_key,
        const BIGNUM** priv_key) {

    /* Retrieve all requested internal values */
    if (pub_key  != NULL) *pub_key  = dsa_key->pub_key;
    if (priv_key != NULL) *priv_key = dsa_key->priv_key;

}
#endif

#ifndef HAVE_DSA_SIG_GET0
void DSA_SIG_get0(const DSA_SIG* dsa_sig, const BIGNUM** r, const BIGNUM** s) {

    /* Retrieve all requested internal values */
    if (r != NULL) *r = dsa_sig->r;
    if (s != NULL) *s = dsa_sig->s;

}
#endif

