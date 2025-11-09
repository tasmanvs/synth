#ifndef FLAC__CONFIG_H
#define FLAC__CONFIG_H

#define FLAC__HAS_OGG 0
#define FLAC__CPU_X86_64 1
#define FLAC__HAS_X86INTRIN 1
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_BSWAP16 1
#define HAVE_BSWAP32 1
#define PACKAGE_VERSION "1.5.0"
#define VERSION "1.5.0"

#ifdef _MSC_VER
#define inline __inline
#define restrict __restrict
#endif

#endif
