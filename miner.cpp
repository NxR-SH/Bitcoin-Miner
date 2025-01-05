#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <boost/asio.hpp>
#include <json/json.h>
#include "sha256.h"
#include "block.h"

#ifdef _WIN32
    #undef _GLIBCXX_USE_C99_QUICK_EXIT
#endif

using namespace boost::asio;
using namespace std;

class StratumClient {
public:
    StratumClient(const string& pool, const string& port, const string& username, const string& password)
        : io_context(), socket(io_context), pool(pool), port(port), username(username), password(password) {}


    void connect() {
    try {
        ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(pool, port);
        boost::asio::connect(socket, endpoints);
        cout << "Connected to pool: " << pool << ":" << port << endl;

        // Authorize miner
        Json::Value request1;
        request1["id"] = 1;
        request1["method"] = "mining.authorize";

        // Construire un tableau pour "params"
        Json::Value params(Json::arrayValue);
        params.append(username);
        params.append(password);

        // Ajouter "params" à l'objet JSON
        request1["params"] = params;

        sendJSON(request1);

        // Subscribe to mining
        Json::Value request;
        request["id"] = 2;
        request["method"] = "mining.subscribe";
        request["params"] = Json::arrayValue; // Initialise "params" comme un tableau vide

        sendJSON(request);

        // Lire la réponse et analyser le JSON
        string response = readResponse();
        Json::Value jsonResponse = parseJSON(response);  // Correction ici

        // Vérifier la méthode "mining.notify"
        if (jsonResponse.isMember("method") && jsonResponse["method"].asString() == "mining.notify") {
            // Appeler la fonction de gestion du job
            handleJob(jsonResponse["params"], 4);  // Exemple avec 4 threads
        } else {
            cerr << "Invalid method in response: " << jsonResponse.toStyledString() << endl;
        }
    } catch (const std::exception& e) {
        cerr << "Error connecting to pool: " << e.what() << endl;
        throw;
    }
}

void mine(int threadCount) {
    while (true) {
        try {
            string response = readResponse();
            Json::Value jsonResponse = parseJSON(response);  // Correction ici

            // Vérification de la méthode du serveur
            if (jsonResponse.isMember("method") && jsonResponse["method"].asString() == "mining.notify") {
                handleJob(jsonResponse["params"], threadCount);
            } else {
                cerr << "Invalid method in response: " << jsonResponse.toStyledString() << endl;
            }

            // Ajout d'un délai pour éviter un usage excessif du CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Pause de 100ms
        } catch (const std::exception& e) {
            cerr << "Error during mining: " << e.what() << endl;
        }
    }
}




private:
    io_context io_context;
    ip::tcp::socket socket;
    string pool, port, username, password;

    atomic<bool> foundSolution{false};
    mutex resultMutex;

    void sendJSON(const Json::Value& json) {
        try {
            Json::StreamWriterBuilder writer;
            string message = Json::writeString(writer, json) + "\n";
            cout << "Sending JSON: " << message << endl;
            write(socket, buffer(message));
        } catch (const std::exception& e) {
            cerr << "Error sending JSON: " << e.what() << endl;
            throw;
        }
    }

    std::string readResponse() {
        try {
            boost::asio::streambuf buffer;
            boost::system::error_code ec;

            boost::asio::read_until(socket, buffer, "\n", ec);

            if (ec) {
                if (ec == boost::asio::error::eof) {
                    cerr << "Error: Server closed the connection (EOF)." << endl;
                } else {
                    cerr << "Error reading response: " << ec.message() << endl;
                }
                throw std::runtime_error("Connection closed or error during reading.");
            }

            std::istream is(&buffer);
            std::string response;
            std::getline(is, response);

            return response;
        } catch (const std::exception& e) {
            std::cerr << "Error reading response: " << e.what() << std::endl;
            throw;
        }
    }

    Json::Value parseJSON(const string& str) {
        Json::Value root;
        Json::CharReaderBuilder builder;
        string errs;
        stringstream ss(str);
        if (!Json::parseFromStream(builder, ss, &root, &errs)) {
            throw runtime_error("Failed to parse JSON: " + errs);
        }
        return root;
    }

    void handleJob(const Json::Value& params, int threadCount) {
        string jobId = params[0].asString();
        string prevhash = params[1].asString();
        string coinb1 = params[2].asString();
        string coinb2 = params[3].asString();
        vector<string> merkleBranch(params[4].size());
        for (unsigned int i = 0; i < params[4].size(); ++i) {
            merkleBranch[i] = params[4][i].asString();
        }
        string version = params[5].asString();
        string nbits = params[6].asString();
        string ntime = params[7].asString();

        cout << "Received new job: " << jobId << endl;

        Block block;
        block.version = stoi(version, nullptr, 16);
        block.timestamp = stoi(ntime, nullptr, 16);
        block.difficulty = stoi(nbits, nullptr, 16);

        foundSolution = false;

        vector<thread> threads;
        uint32_t nonceStart = 0;
        uint32_t nonceRange = UINT32_MAX / threadCount;

        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back(&StratumClient::mineThread, this, block, nonceStart, nonceStart + nonceRange, jobId);
            nonceStart += nonceRange;
        }

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        if (!foundSolution) {
            cout << "No solution found for job: " << jobId << endl;
        }
    }

    void mineThread(Block block, uint32_t startNonce, uint32_t endNonce, const string& jobId) {
        try {
            uint32_t hashOutput[8];
            uint32_t input[16];
            string blockData = block.toString();
            memset(input, 0, sizeof(input));

            for (size_t i = 0; i < blockData.size(); ++i) {
                input[i / 4] = (input[i / 4] << 8) | static_cast<uint8_t>(blockData[i]);
            }

            for (block.nonce = startNonce; block.nonce < endNonce && !foundSolution; ++block.nonce) {
                sha256Hash(input, blockData.size() * 8, hashOutput);

                if (hashOutput[0] < block.difficulty) {
                    lock_guard<mutex> lock(resultMutex);
                    if (!foundSolution) {
                        foundSolution = true;
                        cout << "Solution found by thread: Nonce = " << block.nonce << endl;

                        Json::Value submit;
                        submit["id"] = 3;
                        submit["method"] = "mining.submit";
                        Json::Value params(Json::arrayValue);
                        params.append(username);
                        params.append(jobId);
                        params.append(to_string(block.timestamp));
                        params.append(to_string(block.nonce));
                        submit["params"] = params;
                        sendJSON(submit);
                    }
                    return;
                }
            }
        } catch (const std::exception& e) {
            cerr << "Error in mining thread: " << e.what() << endl;
        }
    }
};

int main() {
    string pool = "btc.viabtc.io";  // Replace with pool address
    string port = "3333";            // Replace with pool port
    string username = "NxRSH27.001"; // Replace with your username
    string password = "270177";      // Replace with your password

    StratumClient client(pool, port, username, password);

    while (true) {
        try {
            cout << "Starting connection to the pool..." << endl;
            client.connect();
            cout << "Starting mining..." << endl;
            client.mine(4);  // Start mining with 4 threads
        } catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
            cerr << "Retrying in 10 seconds..." << endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));  // Attendre avant de réessayer
        }
    }

    return 0;
}
