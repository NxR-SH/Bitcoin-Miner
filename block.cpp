#include <sstream>
#include <iomanip>
#include "block.h"
#include "sha256.h"
#include <cstdint>

std::string Block::toString() const {
    std::stringstream ss;
    ss << "Version: " << version << "\n"
       << "Prev Block Hash: " << prevBlockHash << "\n"
       << "Coinbase1: " << coinbase1 << "\n"
       << "Coinbase2: " << coinbase2 << "\n"
       << "Merkle Branches: ";
    for (const auto& branch : merkleBranches) {
        ss << branch << " ";
    }
    ss << "\nMerkle Root: " << merkleRoot << "\n"
       << "Timestamp: " << timestamp << "\n"
       << "Difficulty: " << difficulty << "\n"
       << "Nonce: " << nonce;
    return ss.str();
}

std::string Block::calculateHash() const {
    std::stringstream ss;
    ss << version << prevBlockHash << merkleRoot << timestamp << difficulty << nonce;
    return sha256(ss.str());
}

bool Block::isValidHash(const std::string& hash) const {
    // Convert nBits to target
    uint32_t target = std::stoul(nBits, nullptr, 16);
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(64) << target;
    std::string targetStr = ss.str();

    // Compare hash with target
    return hash < targetStr;
}