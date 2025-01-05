#ifndef SHA256_H
#define SHA256_H

#include <cstdint>

uint32_t rotateInt(uint32_t inputWord, int numberOfBitsToRotate);
int Ch(int x, int y, int z);
int Maj(int x, int y, int z);
int Sig0f(int x);
int Sig1f(int x);
uint32_t sig0(uint32_t x);
uint32_t sig1(uint32_t x);
void sha256Hash(const unsigned int* input, int len, uint32_t* output);


#endif // SHA256_H
