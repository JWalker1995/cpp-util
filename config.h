#ifndef JWUTIL_CONFIG_H
#define JWUTIL_CONFIG_H

#include <iostream>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace jw_util
{

class Config : public YAML::Node
{
public:
    void load_from_args(unsigned int argc, char **argv)
    {
        if (argc > 1)
        {
            load_file(argv[1]);
        }
        else
        {
            load_file("config.yaml");
        }

        assert(argc > 0);
        for (unsigned int i = 2; i < argc; i++)
        {
            std::string arg = argv[i];
            if (arg[0] != '-')
            {
                std::cerr << "Unknown argument \"" << arg << "\"" << std::endl;
                continue;
            }

            std::string::size_type j = arg.find('=');
            if (j != std::string::npos)
            {
                std::string key = arg.substr(1, j);
                std::string val = arg.substr(j + 1);
                override(key, val);
            }
        }

    }

    void load_file(const std::string &file_path)
    {
        std::ifstream handle(file_path);
        if (handle.is_open())
        {
            std::cout << "Loaded config file \"" << file_path << "\"" << std::endl;
            YAML::Node::operator =(YAML::Load(handle));
        }
        else
        {
            std::cerr << "Cannot open config file \"" << file_path << "\"" << std::endl;
        }
    }

    void override(const std::string &key, const std::string &value)
    {
        // TODO
        assert(false);

        /*
        YAML::Node *node = this;

        std::string::size_type start = 0;
        while (true)
        {
            std::string::size_type end = key.find('.', start);
            node = node->[key.substr(start, end - start)];
            if (end == std::string::npos) {break;}
            start = end + 1;
        }

        node.Assign(value.data());
        */
    }

    static const Config &get_instance() {return inst;}
    static Config &get_mutable_instance() {return inst;}

private:
    static Config inst;
};

}

#endif // JWUTIL_CONFIG_H
