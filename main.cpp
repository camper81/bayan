#include <string>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/name_generator_sha1.hpp>
#include <boost/uuid/name_generator_md5.hpp>
#include <boost/crc.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <array>
#include <iomanip>
#include <stack>

namespace po = boost::program_options;

class Duplicator {

};

struct hash{
    hash(const char* seq);
    virtual void calc(const char* seq, size_t size) = 0;
    virtual bool operator==(hash& other) = 0;
};

struct hash_sha1 : public hash{
   void calc(const char* seq, size_t size) {
       boost::uuids::detail::sha1 h;
       h.process_bytes(seq, size);
   }
   bool operator==(hash& other) {
       other.operator==(*this);
   }

    bool operator==(hash_sha1& other) {
       size_t size = sizeof(calculated_sum) / sizeof(calculated_sum[0]);
       for(size_t i = 0; i < size; ++i)
           if(other.calculated_sum[i] != calculated_sum[i])
               return false;

       return true;
    }

    int calculated_sum[5] {0};
};

struct file_info {
    file_info(const std::string& p) : filepath(p) {}
    std::string filepath;
    std::vector<std::shared_ptr<hash>> hash;
};

struct comparator{
    comparator(std::string path) : fs_(path) {};
    std::fstream fs_;
    bool isOpen = true;
};

struct hash_digits{
    unsigned int digits[5];
    std::string path_;
    hash_digits(const char* str, size_t block_size, std::string path) : path_(path) {
        boost::uuids::detail::sha1 sha;
        sha.process_bytes(str, block_size);
        sha.get_digest(digits);
    }

    bool operator<(const hash_digits& other) const {
        return std::tie(this->digits[0],this->digits[1],this->digits[2],this->digits[3],this->digits[4]) <
                std::tie(other.digits[0],other.digits[1],other.digits[2],other.digits[3],other.digits[4]);
    }
    bool operator==(const hash_digits& other) const {
        return std::tie(this->digits[0],this->digits[1],this->digits[2],this->digits[3],this->digits[4]) ==
               std::tie(other.digits[0],other.digits[1],other.digits[2],other.digits[3],other.digits[4]);
    }
};

void compareFiles(std::vector<file_info>& vec, size_t block_size, size_t chunk_cnt){
    struct file_in{
        std::string path;
        std::fstream* fs;
    };
//    std::list<comparator> list;
    std::vector<char> buf(block_size, 0);
    std::map<hash_digits, int> hash;
    std::map<std::string, std::fstream> opened_files;
    for(auto& elem : vec) {
        opened_files[elem.filepath] = std::fstream(elem.filepath);
    }
    std::vector<file_in> stack;
    std::vector<file_in> stack_n;
    for(auto& elem : vec) {
        stack.push_back({elem.filepath, std::make_shared<std::fstream>(elem.filepath)});
    }

    boost::uuids::detail::sha1 h;
    size_t offset = 0;
    while(--chunk_cnt) {
        std::vector<std::fstream*> stack;
        for(auto& elem : opened_files)
            stack.push_back(&elem.second);
        for(auto& elem: stack) {
            elem->read(&buf[0], block_size);
            h.process_bytes(&buf[0], block_size);
            hash_digits dig(&buf[0], block_size, elem);
            hash[dig]++;
        }
    }

    for(auto& file : stack) {
        file.fs->close();
    }
}

int main(int argc, char** argv){
    std::map<size_t, std::vector<file_info>> blockCnt_fileInfos;

    po::options_description desc;
    desc.add_options()
            ("help", "produce help message")
            ("include,i", po::value< std::vector<std::string> >()
                    ->composing()->multitoken(), "include directories")
            ("exclude,e", po::value< std::vector<std::string> >()
                    ->composing()->multitoken(), "exclude directories")
            ("level,l", po::value< int >()->default_value(0), "scanning level")
            ("hash,h", po::value<std::string>()->default_value("crc32"), "set hash function")
            ("size,s", po::value<int>()->default_value(4), "set block size")
            ;
    po::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);

    notify(vm);

    if(vm.count("include")){
        for(auto& s : vm["include"].as<std::vector<std::string>>())
            std::cout << s << std::endl;
    }
    if(vm.count("exclude")) {
        for (auto &s: vm["exclude"].as<std::vector<std::string>>())
            std::cout << s << std::endl;
    }


    const int block_size = vm["size"].as<int>();
    std::vector<char> buf(block_size, 0);

    boost::filesystem::recursive_directory_iterator it(boost::filesystem::current_path());
    for(auto& d : it){
        std::cout << it.level() << std::endl;
        std::cout << d.path() << std::endl;


        if(boost::filesystem::is_regular(d.path())) {
            blockCnt_fileInfos[boost::filesystem::file_size(d.path())/ block_size].emplace_back(d.path().string());
                boost::uuids::detail::sha1 h;
                std::fstream file(d.path().string());
                file.read(&buf[0], block_size);
                h.process_bytes(&buf[0], block_size);
//                unsigned int digest[5];
//                h.get_digest(digest);
//                for(auto d : digest) {
//                    std::cout << std::hex << std::setfill('0') << std::setw(8) << d << std::endl;
//                }
        }
    }

    for(auto& filevec : blockCnt_fileInfos) {
        if(filevec.second.size() > 1) {
            compareFiles(filevec.second, block_size, filevec.first);
            std::cout << std::endl;
        }
    }

}