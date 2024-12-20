# Demangle

This module implements C++ name demangling for GNU GCC. GNU conforms to the name mangling scheme of the [Itanium C++ ABI](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling).

The library is written in plain C with C99 extensions.

## License

`Demangle` is licensed under the [Apache License version 2](https://www.apache.org/licenses/LICENSE-2.0).

## Work in progress

The current state of the library has had only minimal testing for demangling of &lt;expression&gt; blocks (e.g. within `decltype` specifications). Ternary expressions and parameter lists inside such expressions are currently not handled. A few more (special) operators may be missing. As an aside: these parts also remain "unimplemented" (or partially implemented) because of the lack of good test cases. These parts are quite rare.

## Usage

There is a single function:

    bool demangle(char *plain, size_t size, const char *mangled);

The first parameter, `plain`, is the output (demangled string); the second parameter is the size of this buffer. The third parameter, `mangled`, is the input string.

The function returns `true` on success, and `false` on failure.

## Limitations

* Only Itanium ABI (no support for Microsoft Visual C/C++). More specifically, it focusses on GCC and clang.
* Only C++ (no Java, Rust, ...).
* No support for an extra leading underscore; if you have a compiler that prefixes every symbol with an underscore, you ought to skip it before calling the `demangle` function. (A leading underscore on every symbol is common on COFF files, but not on ELF).

## Why build my own

The canonical implementation for name demangling is `cp-demangle`, originally from the `libiberty` project. While I read the story of how the [early releases of cp-demangle were riddled with bugs](https://fitzgeraldnick.com/2017/02/22/cpp-demangle.html), those were the early versions. The *current* version is probably the most field-tested demangler for GNU C++.

The reason I decided to make my own, is the license: `cp-demangle` is GPL. While I release my demangle library as open-source, I want to be able to use it in commercial projects as well.

As an aside, for testing, I used many of the tests in the file `demangle-expected.txt`, which is part of the `cp-demangle` project. However, I did not look at the source code for `cp-demangle`. I therefore consider this code a clean-room implementation.

## Background information

The official docmentation of the [Itanium C++ ABI](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling) covers the name mangling syntax and semantics in chapter 5.

The Itanium ABI documentation has omissions and a few errors, however. Guillaume Chatelet collected [additional notes](https://github.com/gchatelet/gcc_cpp_mangling_documentation) on the format.

