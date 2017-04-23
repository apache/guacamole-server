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
#include "terminal/palette.h"

const guac_terminal_color guac_terminal_palette[16] = {

    /* Normal colors */
    {0, 0x00, 0x00, 0x00}, /* Black   */
    {1, 0x99, 0x3E, 0x3E}, /* Red     */
    {2, 0x3E, 0x99, 0x3E}, /* Green   */
    {3, 0x99, 0x99, 0x3E}, /* Brown   */
    {4, 0x3E, 0x3E, 0x99}, /* Blue    */
    {5, 0x99, 0x3E, 0x99}, /* Magenta */
    {6, 0x3E, 0x99, 0x99}, /* Cyan    */
    {7, 0x99, 0x99, 0x99}, /* White   */

    /* Intense colors */
    {8,  0x3E, 0x3E, 0x3E}, /* Black   */
    {9,  0xFF, 0x67, 0x67}, /* Red     */
    {10, 0x67, 0xFF, 0x67}, /* Green   */
    {11, 0xFF, 0xFF, 0x67}, /* Brown   */
    {12, 0x67, 0x67, 0xFF}, /* Blue    */
    {13, 0xFF, 0x67, 0xFF}, /* Magenta */
    {14, 0x67, 0xFF, 0xFF}, /* Cyan    */
    {15, 0xFF, 0xFF, 0xFF}, /* White   */

};

int guac_terminal_colorcmp(const guac_terminal_color* a,
        const guac_terminal_color* b) {

    /* Consider red component highest order ... */
    if (a->red != b->red)
        return a->red - b->red;

    /* ... followed by green ... */
    if (a->green != b->green)
        return a->green - b->green;

    /* ... followed by blue */
    if (a->blue != b->blue)
        return a->blue - b->blue;

    /* If all components match, colors are equal */
    return 0;

}

