#pragma once
#include <iostream>
#include <boost/program_options.hpp>

class command_line_parser
{
    using description = boost::program_options::options_description;
    using variables_map = boost::program_options::variables_map;

public:
    /// @brief Конструктор
    /// @param ac Количество параметров в строке запуска
    /// @param av Указатель на параметры строки запуска
    command_line_parser(int ac, char **av, const std::string& program_name)
            : m_desc(program_name), m_argc(ac), m_argv(av) {
        m_desc.add_options()("help,?", "Output this window")("verbose,v", "Output program information");
    }

    /// @brief Если есть параметр   help то вывести окно с помощью
    void check_help(){
        if (m_vm.count("help") || m_vm.empty())
        {
            std::cout << m_desc << "\n";
        }
    }

    void check_verbose(const std::string& program_description, const std::string& version) {
        if (!m_vm.count("verbose"))
            return;
        std::cout << program_description <<". ";
        std::cout << "Version = " << version << "; ";
        std::cout << "Compilation=[" << __DATE__ << " " << __TIME__ << "]";
        std::cout << "\n";
    }

    /// @brief Взять значение параметра запуска [параметр=значение]
    /// @tparam T Тип значения параметра запуска
    /// @param param Параметр в строке запуска
    /// @param value Значение параметра в строке запуска
    /// @return Если параметр был найден то возвращает true
    template <typename T>
    T get_param_value(std::string param)
    {
        if (m_vm.count(param))
            return m_vm[param].as<T>();
        ;
        // Если запрашиваемый параметр не найден то вернуть пустое значение
        return {};
    }

    /// @brief Добавить параметры в список обработки параметров командной строки
    template<typename T>
    void add_options(const std::string& abbreviate, const std::string& value)
    {
            m_desc.add_options()(
                    abbreviate.c_str(), boost::program_options::value<T>(), value.c_str());
    }
    void init() {
        namespace po = boost::program_options;
        po::parsed_options parsed = po::command_line_parser(m_argc, m_argv)
                .options(m_desc)
                .allow_unregistered()
                .run();
        po::store(parsed, m_vm);
        po::notify(m_vm);
    }
private:
    /// @brief Список обрабатываемых параметров командной строки запуска
    description m_desc;
    /// @brief Таблица параметр и значение в командной строке
    boost::program_options::variables_map m_vm;
    int m_argc;
    char** m_argv;
};