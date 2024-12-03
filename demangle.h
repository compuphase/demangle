/* GNU C++ symbol name demangler
 *
 * This decoding module follows the specification of the Itanium C++ ABI,
 * documented at: https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
 *
 * Copyright 2022-2024, CompuPhase
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _DEMANGLE_H
#define _DEMANGLE_H

#include <stdbool.h>

bool demangle(char *plain, size_t size, const char *mangled);

#endif /* _DEMANGLE_H */
