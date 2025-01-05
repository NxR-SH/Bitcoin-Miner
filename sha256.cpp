#include <cstdint>
#include <cstring>

uint32_t rotateInt(uint32_t inputWord, int numberOfBitsToRotate) 
{
    int bitWidth = sizeof(inputWord) * 8;
    // Rotating 32 bits on a 32-bit integer is the same as rotating 0 bits;
    numberOfBitsToRotate = numberOfBitsToRotate % bitWidth;

    uint32_t tempWord = inputWord;
    inputWord = inputWord >> numberOfBitsToRotate;

    // Rotate input to the right and shift the remaining bits to the left
    tempWord = tempWord << (bitWidth - numberOfBitsToRotate);

    return inputWord | tempWord;
}

int Ch(int x, int y, int z)
{
    return ((x & y) ^ (~x & z));
}

int Maj(int x, int y, int z)
{
    return ((x & y) ^ (x & z) ^ (y & z));
}

int Sig0f(int x)
{
    return (rotateInt(x, 2) ^ rotateInt(x, 13) ^ rotateInt(x, 22));
}

int Sig1f(int x)
{
    return (rotateInt(x, 6) ^ rotateInt(x, 11) ^ rotateInt(x, 25));
}

uint32_t sig0(uint32_t x)
{
    return (rotateInt(x, 7) ^ rotateInt(x, 18) ^ (x >> 3));
}

uint32_t sig1(uint32_t x)
{
    return (rotateInt(x, 17) ^ rotateInt(x, 19) ^ (x >> 10));
}

void sha256Hash(const unsigned int* input, int len, uint32_t* output)
{
    uint32_t H_0[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    uint32_t K[64] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                       0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                       0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                       0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                       0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                       0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                       0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                       0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

    int bitlength = len * 8;
    uint32_t message[10000] = {};

    // Fill the message array with input data
    for (int i = 0; i < len; i++) {
        message[i] = input[i];
    }

    // Padding
    message[len] = 0x80;  // Append a 1 bit (0x80 is 1000 0000 in binary)
    int padding_length = (bitlength + 64) % 512;
    if (padding_length != 0) {
        for (int i = len + 1; i < (bitlength + 64) / 8; i++) {
            message[i] = 0;
        }
    }

    // Append the original bit length
    message[(bitlength + 64) / 8 - 1] = bitlength;

    uint32_t W[64];
    uint32_t a, b, c, d, e, f, g, h;

    // Initialize the hash value
    for (int i = 0; i < 8; i++) {
        H_0[i] = H_0[i];
    }

    for (int i = 0; i < 64; i++) {
        W[i] = message[i];
    }

    // Main loop
    for (int i = 0; i < 64; i++) {
        uint32_t T1 = h + Sig1f(e) + Ch(e, f, g) + K[i] + W[i];
        uint32_t T2 = Sig0f(a) + Maj(a, b, c);

        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    // Update the hash values
    for (int i = 0; i < 8; i++) {
        output[i] = H_0[i] + H_0[i];
    }
}
