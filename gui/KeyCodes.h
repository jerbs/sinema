//
// Key Codes
//
// Copyright (C) Joachim Erbs, 2012
//
//    This file is part of Sinema.
//
//    Sinema is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Sinema is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Sinema.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef KEY_CODES_H
#define KEY_CODES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    extern const char* get_key_description(uint16_t code);

#ifdef __cplusplus
}
#endif

#endif