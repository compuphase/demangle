# Demangle

This library implements C++ name demangling for GNU GCC. GNU conforms to the name mangling scheme of the [Itanium C++ ABI](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling).

The library is written in plain C with C99 extensions.

## License

`Demangle` is licensed under the [Apache License version 2](https://www.apache.org/licenses/LICENSE-2.0).

## Work in progress

The current state of the library has the following known deficiencies:

* No support for expressions; literals are handled, but operations on the literals are not.
* No support for decltype().

## Usage

There is a single function:

    int demangle(char *plain, size_t size, const char *mangled);

The first parameter, `plain`, is the output (demangled string); the second parameter is the size of this buffer. The third parameter, `mangled`, is the input string.

The function returns 0 (zero) on failure, and 1 on success.

## Limitations

* Only Itanium ABI (no support for Microsoft Visual C/C++).
* Only C++ (no Java, Rust, ...).
* No support for an extra leading underscore; if you have a compiler that prefixes every symbol with an underscore, you ought to skip it before calling the `demangle` function. (A leading underscore on every symbol is common on COFF files, but not on ELF).

## Why build my own

The canonical implementation for name demangling is `cp-demangle`, originally from the `libiberty` project. While I read the story of how the [early releases of this code were riddled with bugs](https://fitzgeraldnick.com/2017/02/22/cpp-demangle.html), those were the early versions. The *current* version is probably the most field-tested demangler for GNU C++.

The reason I decided to make my own, is the license: `cp-demangle` is GPL. While I release my demangle library as open-source, I want to be able to use it in commercial projects as well.

As an aside, I used the test file for `cp-demangle`, but did not look at the source code. I therefore consider this code a clean-room implementation.

## Background information

The official docmentation of the [Itanium C++ ABI](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling) covers the name mangling syntax and semantics in chapter 5.

The Itanium ABI documentation has omissions and a few errors, however. Guillaume Chatelet collected [additional notes](https://github.com/gchatelet/gcc_cpp_mangling_documentation) on the format.

