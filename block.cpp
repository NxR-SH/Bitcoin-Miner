#include <sstream> // pour std::stringstream
#include "block.h"


std::string Block::toString() const {
    std::stringstream ss;
    ss << "Version: " << version << "\n"
       << "Prev Block Hash: ";
    for (int i = 0; i < 8; i++) {
        ss << std::hex << prevBlockHash[i] << " ";
    }
    ss << "\nMerkle Root: ";
    for (int i = 0; i < 8; i++) {
        ss << std::hex << merkleRoot[i] << " ";
    }
    ss << "\nTimestamp: " << timestamp << "\n"
       << "Difficulty: " << difficulty << "\n"
       << "Nonce: " << nonce;
    return ss.str();
}
