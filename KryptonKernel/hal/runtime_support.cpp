//made by AI, declarations of memset and memcpy functions for UEFI runtime
//in order to support UEFI, these functions are used to set memory and copy memory respectively, they are implemented in a simple way using loops to iterate over the bytes of the memory regions.
//note made by saphhic.

extern "C" void* memset(void* destination, int value, unsigned long long size) {
    unsigned char* bytes = static_cast<unsigned char*>(destination);
    for (unsigned long long i = 0; i < size; ++i) {
        bytes[i] = static_cast<unsigned char>(value);
    }

    return destination;
}

extern "C" void* memcpy(void* destination, const void* source, unsigned long long size) {
    unsigned char* dst = static_cast<unsigned char*>(destination);
    const unsigned char* src = static_cast<const unsigned char*>(source);
    for (unsigned long long i = 0; i < size; ++i) {
        dst[i] = src[i];
    }

    return destination;
}
