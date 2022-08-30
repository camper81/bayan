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
    hash_digits(const char* str, size_t block_size) {
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

void compareFiles(std::vector<std::string>& filenames, size_t block_size, size_t chunk_cnt){
    struct file {
        std::fstream* fs;
    };

    struct identity {
        int same_files;
        std::vector<std::pair<std::fstream*, std::string>> open_files;
    };

    std::vector<char> buf(block_size, 0);

    std::map<std::string, std::fstream> opened_files;

    for(auto& elem : filenames) {
        opened_files[elem] = std::fstream(elem);
    }

    std::vector<std::vector<std::pair<std::fstream*, std::string>>> stack;
    {
        std::vector<std::pair<std::fstream*, std::string>> sub_vector;
        for(auto& elem : opened_files) {
            sub_vector.emplace_back(&elem.second, elem.first);
        }
        stack.push_back(sub_vector);
    }

    boost::uuids::detail::sha1 h;
    while(--chunk_cnt) {
        std::map<hash_digits, identity> hash;
        for(auto& sub_vector: stack) {
            for(auto& elem: sub_vector) {
                elem.first->read(&buf[0], block_size);
                h.process_bytes(&buf[0], block_size);
                hash_digits dig(&buf[0], block_size);
                hash[dig].same_files++;
                hash[dig].open_files.push_back(elem);
            }
        }

        stack.clear();

        for(auto& elem: hash) {
            if(elem.second.same_files > 1) {
                stack.emplace_back(elem.second.open_files);
            }
        }

    }

    for(auto& filevec : stack) {
        for(auto& file : filevec) {
            std::cout << file.second << std::endl;
        }
        std::cout << std::endl;
    }

    for(auto& file : opened_files) {
        file.second.close();
    }
}

int main(int argc, char** argv){
    std::map<size_t, std::vector<std::string>> cntBlocksToFilename;

    po::options_description desc;
    desc.add_options()
            ("help", "produce help message")
            ("include,i", po::value< std::vector<std::string> >()
                    ->composing()->multitoken(), "include directories")
            ("exclude,e", po::value< std::vector<std::string> >()
                    ->composing()->multitoken(), "exclude directories")
            ("level,l", po::value< int >(), "scanning level")
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

    std::vector<boost::filesystem::recursive_directory_iterator> check_directories;
    if(vm.count("include"))
        for(auto& dir : vm["include"].as<std::vector<std::string>>()) {
            check_directories.emplace_back(dir);
        }

    if(check_directories.empty())
            check_directories.emplace_back(boost::filesystem::current_path());

    for(auto& dir : check_directories) {
        for(auto& r_it: dir){
            if(vm.count("level") && (dir.level() > vm["level"].as<int>()))
                continue;
            std::cout << dir.level() << std::endl;
//            std::cout << r_it.path().branch_path().string() << std::endl;
            if(vm.count("exclude"))
                if(std::find(vm["exclude"].as<std::vector<std::string>>().begin(),
                             vm["exclude"].as<std::vector<std::string>>().end(),
                             r_it.path().branch_path()) != vm["exclude"].as<std::vector<std::string>>().end())
                    continue;

            std::cout << r_it.path().branch_path() << std::endl;

            if(boost::filesystem::is_regular(r_it.path())) {
                cntBlocksToFilename[boost::filesystem::file_size(r_it.path())/ block_size].emplace_back(r_it.path().string());

            }
        }
    }

    for(auto& filevec : cntBlocksToFilename) {
        // with the same numbers of blocks
        if(filevec.second.size() > 1) {
            compareFiles(filevec.second, block_size, filevec.first);
            std::cout << std::endl;
        }
    }

}