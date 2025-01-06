#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <string>

uint32_t rotateInt(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

uint32_t Sig0f(uint32_t x) {
    return rotateInt(x, 2) ^ rotateInt(x, 13) ^ rotateInt(x, 22);
}

uint32_t Sig1f(uint32_t x) {
    return rotateInt(x, 6) ^ rotateInt(x, 11) ^ rotateInt(x, 25);
}

uint32_t sig0(uint32_t x) {
    return rotateInt(x, 7) ^ rotateInt(x, 18) ^ (x >> 3);
}

uint32_t sig1(uint32_t x) {
    return rotateInt(x, 17) ^ rotateInt(x, 19) ^ (x >> 10);
}

void sha256Hash(const unsigned int* input, int len, uint32_t* output)
{
    uint32_t H[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
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
        for (int i = len + 1; i < ((bitlength + 64 + 511) / 512) * 64 - 8; i++) {
            message[i] = 0;
        }
    }

    // Append the original bit length
    message[((bitlength + 64 + 511) / 512) * 64 - 1] = bitlength;

    uint32_t W[64];
    uint32_t a, b, c, d, e, f, g, h;

    // Process each 512-bit chunk
    for (int chunk = 0; chunk < ((bitlength + 64 + 511) / 512); chunk++) {
        // Prepare the message schedule
        for (int i = 0; i < 16; i++) {
            W[i] = message[chunk * 64 + i];
        }
        for (int i = 16; i < 64; i++) {
            W[i] = sig1(W[i - 2]) + W[i - 7] + sig0(W[i - 15]) + W[i - 16];
        }

        // Initialize working variables to current hash value
        a = H[0];
        b = H[1];
        c = H[2];
        d = H[3];
        e = H[4];
        f = H[5];
        g = H[6];
        h = H[7];

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

        // Add the compressed chunk to the current hash value
        H[0] += a;
        H[1] += b;
        H[2] += c;
        H[3] += d;
        H[4] += e;
        H[5] += f;
        H[6] += g;
        H[7] += h;
    }

    // Produce the final hash value (big-endian)
    for (int i = 0; i < 8; i++) {
        output[i] = H[i];
    }
}

std::string sha256(const std::string& input) {
    // Convert input string to unsigned int array
    std::vector<unsigned int> data(input.begin(), input.end());

    // Call the sha256Hash function
    uint32_t hash[8];
    sha256Hash(data.data(), data.size(), hash);

    // Convert the hash to a hexadecimal string
    std::stringstream ss;
    for (int i = 0; i < 8; i++) {
        ss << std::hex << std::setw(8) << std::setfill('0') << hash[i];
    }
    return ss.str();
}