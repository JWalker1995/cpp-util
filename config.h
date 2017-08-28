#ifndef JWUTIL_CONFIG_H
#define JWUTIL_CONFIG_H

#include <iostream>
#include <fstream>
#include <yaml-cpp/yaml.h>

#include "jw_util/context.h"

namespace jw_util
{

class Config {
public:
    class MapIterator {
    public:
        MapIterator(const Config &config) {
            std::vector<YAML::Node>::const_iterator i = config.sources.cbegin();
            while (i != config.sources.cend()) {
                YAML::Node::const_iterator j = i->begin();
                while (j != i->end()) {
                    findItem(j->first.as<std::string>()).second.sources.push_back(j->second);
                    j++;
                }
                i++;
            }
        }

        bool has() const {
            return !items.empty();
        }

        const std::string &key() const {
            return items.back().first;
        }

        const Config &val() const {
            return items.back().second;
        }

        void advance() {
            items.pop_back();
        }

    private:
        std::pair<std::string, Config> &findItem(std::string &&key) {
            std::vector<std::pair<std::string, Config>>::iterator i = items.begin();
            while (i != items.end()) {
                if (i->first == key) {
                    return *i;
                }
                i++;
            }

            // If we didn't find an existing item with the key, make one
            items.emplace_back();
            items.back().first = std::move(key);
            return items.back();
        }

        std::vector<std::pair<std::string, Config>> items;
    };

    void addSourceFromArgs(int argc, char **argv) {
        if (argc == 2) {
            loadFile(argv[1]);
        } else {
            loadFile("config.yaml");
        }
    }

    void loadFile(const std::string &file_path) {
        std::ifstream handle(file_path);
        if (handle.is_open()) {
            std::cout << "Loaded config file \"" << file_path << "\"" << std::endl;
            sources.push_back(YAML::Load(handle));
        } else {
            std::cerr << "Cannot open config file \"" << file_path << "\"" << std::endl;
        }
    }

    Config operator[](const std::string &key) const {
        Config res;
        res.sources.reserve(sources.size());

        std::vector<YAML::Node>::const_iterator i = sources.cbegin();
        while (i != sources.cend()) {
            YAML::Node canidate = i->operator[](key);
            if (canidate.IsDefined()) {
                res.sources.push_back(canidate);
            }
            i++;
        }

        return res;
    }

    template <typename CastType>
    operator CastType() const {
        return sources.back().as<CastType>();
    }

    static Config prop(Context<Config> context, const std::string &key) {
        return context.get<Config>()[key];
    }

private:
    std::vector<YAML::Node> sources;
    // Later sources have higher priority
};

}

#endif // JWUTIL_CONFIG_H
