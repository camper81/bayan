#include <string>
#include <boost/program_options.hpp>
#include <iostream>
#include <vector>

namespace po = boost::program_options;

int main(int argc, char** argv){
    po::options_description desc;
    desc.add_options()
            ("help", "produce help message")
            ("include,i", po::value< std::vector<std::string> >()
                    ->composing()->multitoken(), "include directories")
            ("exclude,e", po::value< std::vector<std::string> >()
                    ->composing()->multitoken(), "exclude directories")
            ("level,l", po::value< int >()->default_value(0), "scanning level")
            ;
    po::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);

    notify(vm);

    for(auto& s : vm["exclude"].as<std::vector<std::string>>())
        std::cout << s << std::endl;
}