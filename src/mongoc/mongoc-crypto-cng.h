/*
 * Copyright 2016 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#ifdef MONGOC_ENABLE_CRYPTO_CNG

#ifndef MONGOC_CRYPTO_CNG_H
#define MONGOC_CRYPTO_CNG_H


#include "mongoc-macros.h"
#include "mongoc-config.h"


BSON_BEGIN_DECLS

MONGOC_EXPORT (void)
mongoc_crypto_cng_hmac_sha1 (mongoc_crypto_t *crypto,
                             const void *key,
                             int key_len,
                             const unsigned char *d,
                             int n,
                             unsigned char *md /* OUT */);

MONGOC_EXPORT (bool)
mongoc_crypto_cng_sha1 (mongoc_crypto_t *crypto,
                        const unsigned char *input,
                        const size_t input_len,
                        unsigned char *output /* OUT */);

BSON_END_DECLS

#endif /* MONGOC_CRYPTO_CNG_H */
#endif /* MONGOC_ENABLE_CRYPTO_CNG */
