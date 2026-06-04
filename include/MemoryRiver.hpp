#ifndef BPT_MEMORYRIVER_HPP
#define BPT_MEMORYRIVER_HPP

#include <filesystem>
#include <fstream>

using std::fstream;
using std::ifstream;
using std::ofstream;
using std::string;

template <class T, int info_len = 2>
class MemoryRiver {
private:
    mutable fstream file;
    string file_name;
    int sizeofT = sizeof(T);
    int header_size = info_len * static_cast<int>(sizeof(int));

    bool ensureOpenRW() const {
        if (file.is_open())
            return true;
        if (file_name.empty())
            return false;
        file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            file.open(file_name, std::ios::out | std::ios::binary);
            if (!file.is_open())
                return false;
            file.close();
            file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        }
        return file.is_open();
    }

public:
    MemoryRiver() = default;

    MemoryRiver(const string &file_name) : file_name(file_name) {}

    void initialise(string FN = "") {
        if (FN != "")
            file_name = FN;
        if (file_name.empty())
            return;
        bool exists = std::filesystem::exists(file_name);
        file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            file.open(file_name, std::ios::out | std::ios::binary);
            file.close();
            file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        }
        file.seekg(0, std::ios::end);
        std::streampos size = file.tellg();
        if (!exists || size < static_cast<std::streampos>(info_len * sizeof(int))) {
            file.close();
            file.open(file_name, std::ios::out | std::ios::binary | std::ios::trunc);
            int tmp = 0;
            for (int i = 0; i < info_len; ++i) {
                file.write(reinterpret_cast<char *>(&tmp), sizeof(int));
            }
            file.close();
            file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
        }
    }

    // 读出第n个int的值赋给tmp，1_base
    void get_info(int &tmp, int n) const {
        if (n < 1 || n > info_len)
            return;
        if (!ensureOpenRW())
            return;
        file.seekg((n - 1) * static_cast<int>(sizeof(int)), std::ios::beg);
        file.read(reinterpret_cast<char *>(&tmp), sizeof(int));
    }

    // 将tmp写入第n个int的位置，1_base
    void write_info(int tmp, int n) {
        if (n < 1 || n > info_len)
            return;
        if (!ensureOpenRW())
            return;
        file.seekp((n - 1) * static_cast<int>(sizeof(int)), std::ios::beg);
        file.write(reinterpret_cast<char *>(&tmp), sizeof(int));
    }

    // 在文件合适位置写入类对象t，并返回写入的位置索引index
    // 位置索引意味着当输入正确的位置索引index，在以下三个函数中都能顺利的找到目标对象进行操作
    // 位置索引index可以取为对象写入的起始位置
    int write(T &t) {
        if (!ensureOpenRW())
            return -1;
        file.seekp(0, std::ios::end);
        int index = static_cast<int>(file.tellp());
        file.write(reinterpret_cast<char *>(&t), sizeofT);
        file.flush();
        return index;
    }

    // 用t的值更新位置索引index对应的对象，保证调用的index都是由write函数产生
    void update(T &t, const int index) {
        if (!ensureOpenRW())
            return;
        file.seekp(index, std::ios::beg);
        file.write(reinterpret_cast<char *>(&t), sizeofT);
        file.flush();
    }

    // 读出位置索引index对应的T对象的值并赋值给t，保证调用的index都是由write函数产生
    void read(T &t, const int index) const {
        if (!ensureOpenRW())
            return;
        file.seekg(index, std::ios::beg);
        file.read(reinterpret_cast<char *>(&t), sizeofT);
    }

    // 删除位置索引index对应的对象(不涉及空间回收时，可忽略此函数)，保证调用的index都是由write函数产生
    void Delete(int index) {
        /* your code here */
        // 不涉及空间回收，空实现

        // file.open(file_name, ifstream::in | ofstream::out | fstream::binary);
        // if (!file.is_open()) {
        //     return;
        // }
        // file.seekp(index);
        // file.write()
    }
};

#endif // BPT_MEMORYRIVER_HPP