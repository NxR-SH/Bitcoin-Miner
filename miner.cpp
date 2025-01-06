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
    StratumClient(const string& poolURL, const string& username, const string& password)
        : io_context(), socket(io_context), poolURL(poolURL), username(username), password(password) {
        parsePoolURL(poolURL, pool, port);
    }

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
            Json::Value params(Json::arrayValue);
            params.append(username);
            params.append(password);
            request1["params"] = params;
            sendJSON(request1);

            // Subscribe to mining
            Json::Value request;
            request["id"] = 2;
            request["method"] = "mining.subscribe";
            request["params"] = Json::arrayValue;
            sendJSON(request);

            string response = readResponse();
            Json::Value jsonResponse = parseJSON(response);

            if (jsonResponse.isMember("method") && jsonResponse["method"].asString() == "mining.notify") {
                handleJob(jsonResponse["params"], 4);
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
                Json::Value jsonResponse = parseJSON(response);

                if (jsonResponse.isMember("method") && jsonResponse["method"].asString() == "mining.notify") {
                    handleJob(jsonResponse["params"], threadCount);
                } else {
                    cerr << "Invalid method in response: " << jsonResponse.toStyledString() << endl;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } catch (const std::exception& e) {
                cerr << "Error during mining: " << e.what() << endl;
            }
        }
    }

private:
    io_context io_context;
    ip::tcp::socket socket;
    string poolURL, pool, port, username, password;
    atomic<bool> foundSolution{false};
    mutex resultMutex;

    void parsePoolURL(const string& url, string& host, string& port) {
        size_t protocolEnd = url.find("://");
        size_t colonPos = url.rfind(":");
        if (protocolEnd != string::npos && colonPos != string::npos && colonPos > protocolEnd) {
            host = url.substr(protocolEnd + 3, colonPos - protocolEnd - 3);
            port = url.substr(colonPos + 1);
        } else {
            throw runtime_error("Invalid pool URL format");
        }
    }

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

    string readResponse() {
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
                throw runtime_error("Connection closed or error during reading.");
            }

            istream is(&buffer);
            string response;
            getline(is, response);
            return response;

        } catch (const std::exception& e) {
            cerr << "Exception in readResponse: " << e.what() << endl;
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
        if (params.size() < 3) {
            cerr << "Invalid job parameters" << endl;
            return;
        }

        string jobId = params[0].asString();
        string prevBlockHash = params[1].asString();
        string coinbase1 = params[2].asString();
        string coinbase2 = params[3].asString();
        vector<string> merkleBranches;
        for (unsigned int i = 4; i < params.size() - 2; ++i) {
            merkleBranches.push_back(params[i].asString());
        }
        string version = params[params.size() - 2].asString();
        string nBits = params[params.size() - 1].asString();

        Block block;
        block.prevBlockHash = prevBlockHash;
        block.coinbase1 = coinbase1;
        block.coinbase2 = coinbase2;
        block.merkleBranches = merkleBranches;
        block.version = version;
        block.nBits = nBits;

        vector<thread> threads;
        uint32_t nonceRange = 0xFFFFFFFF / threadCount;
        for (int i = 0; i < threadCount; ++i) {
            uint32_t startNonce = i * nonceRange;
            uint32_t endNonce = (i + 1) * nonceRange;
            threads.emplace_back(&StratumClient::mineThread, this, block, startNonce, endNonce, jobId);
        }

        for (auto& t : threads) {
            t.join();
        }
    }

    void mineThread(Block block, uint32_t startNonce, uint32_t endNonce, const string& jobId) {
        for (uint32_t nonce = startNonce; nonce < endNonce; ++nonce) {
            if (foundSolution.load()) {
                return;
            }

            block.nonce = nonce;
            string hash = block.calculateHash();

            if (block.isValidHash(hash)) {
                lock_guard<mutex> lock(resultMutex);
                if (!foundSolution.load()) {
                    foundSolution.store(true);
                    cout << "Solution found! Nonce: " << nonce << ", Hash: " << hash << endl;
                    // Submit the solution to the pool
                    Json::Value result;
                    result["id"] = 4;
                    result["method"] = "mining.submit";
                    Json::Value params(Json::arrayValue);
                    params.append(username);
                    params.append(jobId);
                    params.append(nonce);
                    result["params"] = params;
                    sendJSON(result);
                }
                return;
            }
        }
    }
};

int main() {
    string poolURL = "stratum+tcp://btc.viabtc.io:3333";
    string username = "NxRSH27";
    string password = "270177";

    StratumClient client(poolURL, username, password);

    while (true) {
        try {
            cout << "Starting connection to the pool..." << endl;
            client.connect();
            cout << "Starting mining..." << endl;
            client.mine(4);
        } catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
            cerr << "Retrying in 10 seconds..." << endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    return 0;
}