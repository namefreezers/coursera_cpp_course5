#include <map>
#include <iostream>
#include <string>
#include <vector>

// Serialization

template<typename T>
void Serialize(T pod, std::ostream &out) {
    out.write(reinterpret_cast<const char *>(&pod), sizeof(pod));
}

template<typename T1, typename T2>
void Serialize(const std::map<T1, T2> &data, std::ostream &out);

void Serialize(const std::string &str, std::ostream &out) {
    Serialize(str.size(), out);
    out.write(str.c_str(), str.size());
}

template<typename T>
void Serialize(const std::vector<T> &data, std::ostream &out) {
    Serialize(data.size(), out);
    for (const auto &item : data) {
        Serialize(item, out);
    }
}

template<typename T1, typename T2>
void Serialize(const std::map<T1, T2> &data, std::ostream &out) {
    Serialize(data.size(), out);
    for (const auto&[k, v] : data) {
        Serialize(k, out);
        Serialize(v, out);
    }
}

// Deserialization

template<typename T>
void Deserialize(std::istream &in, T &pod) {
    in.read(reinterpret_cast<char *>(&pod), sizeof(pod));
}

template<typename T1, typename T2>
void Deserialize(std::istream &in, std::map<T1, T2> &data);

void Deserialize(std::istream &in, std::string &str) {
    size_t read_str_size;
    Deserialize(in, read_str_size);
    str.resize(read_str_size);
    in.read(str.data(), read_str_size);
}

template<typename T>
void Deserialize(std::istream &in, std::vector<T> &data) {
    size_t read_data_size;
    Deserialize(in, read_data_size);
    data.resize(read_data_size);
    for (auto &item : data) {
        Deserialize(in, item);
    }
}

template<typename T1, typename T2>
void Deserialize(std::istream &in, std::map<T1, T2> &data) {
    size_t read_data_size;
    Deserialize(in, read_data_size);
    data.clear();
    for (int i = 0; i < read_data_size; ++i) {
        T1 key;
        Deserialize(in, key);
        Deserialize(in, data[key]);
    }
}
