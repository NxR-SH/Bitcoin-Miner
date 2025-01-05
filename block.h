#ifndef BLOCK_H
#define BLOCK_H

#include <cstdint>
#include <string>

struct Block {
    uint32_t version;
    uint32_t prevBlockHash[8];
    uint32_t merkleRoot[8];
    uint32_t timestamp;
    uint32_t difficulty;
    uint32_t nonce;

    std::string toString() const; // Déclaration de la méthode
};

#endif // BLOCK_H
