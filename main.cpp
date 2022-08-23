#include <string>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/name_generator_sha1.hpp>
#include <boost/uuid/name_generator_md5.hpp>
#include <boost/crc.hpp>
#include <iostream>
#include <vector>
#include <array>

namespace po = boost::program_options;

class Duplicator{

};

struct file_info {
    std::vector<uint64_t> hash;
};

int main(int argc, char** argv){
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

    {
        const int block_size = vm["size"].as<int>();
        std::vector<char> buf(block_size, 0);

        boost::filesystem::recursive_directory_iterator it(boost::filesystem::current_path());
        for(auto& d : it){
            std::cout << it.level() << std::endl;
            std::cout << d.path() << std::endl;
            if(boost::filesystem::is_regular(d.path())) {
                std::fstream file(d.path().string());
                file.read(&buf[0], block_size);
                boost::uuids::name_generator_sha1
            }

        }
    }
}