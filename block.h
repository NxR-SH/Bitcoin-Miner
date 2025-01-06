#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <vector>
#include <cstdint>

struct Block {
    std::string prevBlockHash;
    std::string coinbase1;
    std::string coinbase2;
    std::vector<std::string> merkleBranches;
    std::string version;
    std::string nBits;
    uint32_t nonce;
    std::string merkleRoot;
    uint32_t timestamp;
    uint32_t difficulty;

    std::string calculateHash() const;
    bool isValidHash(const std::string& hash) const;
    std::string toString() const;
};

#endif // BLOCK_H