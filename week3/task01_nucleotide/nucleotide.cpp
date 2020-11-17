#include <cstddef>
#include <cstdint>
#include <unordered_map>

using namespace std;

struct Nucleotide {
    char Symbol;
    size_t Position;
    int ChromosomeNum;
    int GeneNum;
    bool IsMarked;
    char ServiceInfo;
};


struct CompactNucleotide {
    uint32_t Position;
    uint32_t Symbol: 2;
    uint32_t ChromosomeNum: 6;
    uint32_t GeneNum: 15;
    uint32_t IsMarked: 1;
    uint32_t ServiceInfo: 8;
};


bool operator==(const Nucleotide &lhs, const Nucleotide &rhs) {
    return (lhs.Symbol == rhs.Symbol)
           && (lhs.Position == rhs.Position)
           && (lhs.ChromosomeNum == rhs.ChromosomeNum)
           && (lhs.GeneNum == rhs.GeneNum)
           && (lhs.IsMarked == rhs.IsMarked)
           && (lhs.ServiceInfo == rhs.ServiceInfo);
}


CompactNucleotide Compress(const Nucleotide &n) {
    static unordered_map<char, uint32_t> symbol_nums = {
            {'A', 0},
            {'T', 1},
            {'G', 2},
            {'C', 3}
    };

    return CompactNucleotide{
            .Position = static_cast<uint32_t>(n.Position),
            .Symbol = symbol_nums.at(n.Symbol),
            .ChromosomeNum = static_cast<uint32_t>(n.ChromosomeNum),
            .GeneNum = static_cast<uint32_t>(n.GeneNum),
            .IsMarked = n.IsMarked,
            .ServiceInfo = static_cast<uint32_t>(n.ServiceInfo)
    };
};


Nucleotide Decompress(const CompactNucleotide &cn) {
    static array<char, 4> symbols = {'A','T','G','C'};

    return Nucleotide{
            .Symbol = symbols.at(cn.Symbol),
            .Position = cn.Position,
            .ChromosomeNum = cn.ChromosomeNum,
            .GeneNum = cn.GeneNum,
            .IsMarked = static_cast<bool>(cn.IsMarked),
            .ServiceInfo = static_cast<char>(cn.ServiceInfo)
    };
}
