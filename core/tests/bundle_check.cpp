#include "bundle.hpp"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

std::vector<uint8_t> read(const char *path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

// Used by the Python end-to-end test against real signed build artifacts.
int main(int argc, char **argv)
{
    if (argc != 5)
        return 2;
    const auto manifest = read(argv[1]);
    const auto corpus = read(argv[2]);
    const auto digest = read(argv[3]);
    const auto key = read(argv[4]);
    if (digest.size() != 32 || key.size() != 32)
        return 2;
    cicala::BundlePlan plan{};
    auto result = cicala::plan_bundle(reinterpret_cast<const char *>(manifest.data()),
                                      manifest.size(), "en", "0.0.0", plan);
    cicala::Qdb q;
    if (result == cicala::BundleResult::Ready)
        result =
            cicala::verify_bundle(plan, corpus.data(), corpus.size(), digest.data(), key.data(), q);
    std::printf("%d\n", static_cast<int>(result));
    return result == cicala::BundleResult::Ready ? 0 : 1;
}
