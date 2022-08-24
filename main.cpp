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

void compareFiles(std::vector<file_info>& vec, size_t block_size){
    std::list<comparator> list;
    std::vector<char> buf(block_size, 0);
    std::map<boost::uuids::detail::sha1, int> hash;


    for(auto& elem : vec) {
        list.emplace_back(elem.filepath);
    }

    boost::uuids::detail::sha1 h;
    bool notEndOfFile = true;

    while(notEndOfFile) {
        for(auto& elem : list) {
            boost::uuids::detail::sha1 h;
            elem.fs_.read(&buf[0], block_size);
            h.process_bytes(&buf[0], block_size);
            hash[h]++;
        }

    }



    for(auto& file : list) {
        file.fs_.close();
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
            compareFiles(filevec.second, block_size);
            std::cout << std::endl;
        }
    }

}