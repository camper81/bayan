#include <iostream>
#include <vector>
#include <string>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/name_generator_sha1.hpp>
#include <boost/uuid/name_generator_md5.hpp>
#include <boost/regex.hpp>

struct hash_md5_digits{
    unsigned int digits[4];
    hash_md5_digits(const char* str, size_t block_size) {
        boost::uuids::detail::md5 sha;
        sha.process_bytes(str, block_size);
        sha.get_digest(digits);
    }

    bool operator<(const hash_md5_digits& other) const {
        return std::tie(this->digits[0],this->digits[1],this->digits[2],this->digits[3]) <
               std::tie(other.digits[0],other.digits[1],other.digits[2],other.digits[3]);
    }
    bool operator==(const hash_md5_digits& other) const {
        return std::tie(this->digits[0],this->digits[1],this->digits[2],this->digits[3]) ==
               std::tie(other.digits[0],other.digits[1],other.digits[2],other.digits[3]);
    }
};

struct hash_sha_digits{
    unsigned int digits[5];
    hash_sha_digits(const char* str, size_t block_size) {
        boost::uuids::detail::sha1 sha;
        sha.process_bytes(str, block_size);
        sha.get_digest(digits);
    }

    bool operator<(const hash_sha_digits& other) const {
        return std::tie(this->digits[0],this->digits[1],this->digits[2],this->digits[3],this->digits[4]) <
                std::tie(other.digits[0],other.digits[1],other.digits[2],other.digits[3],other.digits[4]);
    }
    bool operator==(const hash_sha_digits& other) const {
        return std::tie(this->digits[0],this->digits[1],this->digits[2],this->digits[3],this->digits[4]) ==
               std::tie(other.digits[0],other.digits[1],other.digits[2],other.digits[3],other.digits[4]);
    }
};

template<typename HASH_ALGORITHM>
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
        std::map<HASH_ALGORITHM, identity> hash;
        for(auto& sub_vector: stack) {
            for(auto& elem: sub_vector) {
                elem.first->read(&buf[0], block_size);
                h.process_bytes(&buf[0], block_size);
                HASH_ALGORITHM dig(&buf[0], block_size);
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

    for(auto& filevec: stack) {
        std::cout << std::endl;
        for(auto& file : filevec) {
            std::cout << file.second << std::endl;
        }
    }

    for(auto& file : opened_files) {
        file.second.close();
    }
}

int main(int argc, char** argv){
    namespace po = boost::program_options;

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
            ("pattern,p", po::value<std::string>(), "pattern for scanning file example \"text(\\\\d+).txt\"")
            ;
    po::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);

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
                if(vm.count("exclude"))
                    if(std::find(vm["exclude"].as<std::vector<std::string>>().begin(),
                                 vm["exclude"].as<std::vector<std::string>>().end(),
                                 r_it.path().branch_path()) != vm["exclude"].as<std::vector<std::string>>().end())
                        continue;

               if(vm.count("pattern")) {
                   try {

                       boost::regex pattern(vm["pattern"].as<std::string>());
                       if (!boost::regex_match(r_it.path().filename().string(), pattern)) {

                           continue;
                       }

                    } catch(std::exception& ex) {
                       std::cout << ex.what() << std::endl;
                    }
               }

               if(boost::filesystem::is_regular(r_it.path())) {
                   cntBlocksToFilename[boost::filesystem::file_size(r_it.path())/ block_size].emplace_back(r_it.path().string());
               }
            }
        }


    for(auto& filevec : cntBlocksToFilename) {
        // with the same numbers of blocks
        if(filevec.second.size() > 1) {
            if(vm.count("hash") && vm["hash"].as<std::string>() == "md5")
                compareFiles<hash_md5_digits>(filevec.second, block_size, filevec.first);
            else
                compareFiles<hash_sha_digits>(filevec.second, block_size, filevec.first);
        }
    }

}