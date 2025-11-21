#include <iostream>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <openssl/evp.h>

using namespace std;
namespace fs = filesystem;

class HardLinkManager {
private:
    string calculateSHA1(const string& filepath) {
        ifstream file(filepath, ios::binary);
        if (!file) {
            return "";
        }

        EVP_MD_CTX* context = EVP_MD_CTX_new();
        EVP_DigestInit_ex(context, EVP_sha1(), nullptr);

        char buffer[4096];
        while (file.read(buffer, sizeof(buffer))) {
            EVP_DigestUpdate(context, buffer, file.gcount());
        }
        EVP_DigestUpdate(context, buffer, file.gcount());

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hashLength;
        EVP_DigestFinal_ex(context, hash, &hashLength);
        EVP_MD_CTX_free(context);

        string result;
        for (unsigned int i = 0; i < hashLength; ++i) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02x", hash[i]);
            result += buf;
        }
        return result;
    }

public:
    void processDirectory(const string& directoryPath) {
        unordered_map<string, vector<string>> hashToFiles;

        for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                string filepath = entry.path().string();
                string hash = calculateSHA1(filepath);
                if (!hash.empty()) {
                    hashToFiles[hash].push_back(filepath);
                }
            }
        }

        for (const auto& [hash, files] : hashToFiles) {
            if (files.size() > 1) {
                const string& originalFile = files[0];
                
                for (size_t i = 1; i < files.size(); ++i) {
                    const string& duplicateFile = files[i];
                    fs::remove(duplicateFile);
                    fs::create_hard_link(originalFile, duplicateFile);
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <directory_path>" << endl;
        return 1;
    }

    string directoryPath = argv[1];
    HardLinkManager manager;
    manager.processDirectory(directoryPath);

    return 0;
}