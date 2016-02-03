#ifndef HUFFMANMODEL_H
#define HUFFMANMODEL_H

#include <assert.h>
#include <array>
#include <queue>
#include <limits.h>

#include "fastmath.h"

namespace jw_util
{

template <unsigned int outputs>
class HuffmanModel
{
public:
    class Builder
    {
    public:
        void add_output(unsigned int value, unsigned int freq)
        {
            nodes.push(new Node(freq, value));
        }

        void compile(HuffmanModel &model)
        {
            assert(nodes.size() == outputs);

            while (nodes.size() >= 2)
            {
                const Node *first = nodes.top();
                nodes.pop();

                const Node *second = nodes.top();
                nodes.pop();

                unsigned int freq_sum = first->freq + second->freq;
                nodes.push(new Node(freq_sum, first, second));
            }

            unsigned int read_tree_pos = 0;
            descend(model, read_tree_pos, 0, 0, nodes.top());
            assert(read_tree_pos == model.read_tree.size());

            nodes.pop();
            assert(nodes.empty());
        }

    private:
        struct Node
        {
            Node(unsigned int freq, unsigned int value)
                : freq(freq)
                , value(value)
                , children{0, 0}
            {}

            Node(unsigned int freq, const Node *child1, const Node *child2)
                : freq(freq)
                , children{child1, child2}
            {}

            unsigned int freq;
            unsigned int value;
            const Node *children[2];

            struct Comparator
            {
                bool operator() (const Node *a, const Node *b) const
                {
                    return a->freq > b->freq;
                }
            };
        };

        std::priority_queue<Node*, std::vector<Node*>, typename Node::Comparator> nodes;

        static void descend(HuffmanModel &model, unsigned int &read_tree_pos, unsigned int path, unsigned int path_bits, const Node *node)
        {
            unsigned int prev_read_tree_pos = read_tree_pos;
            assert(prev_read_tree_pos < model.read_tree.size());
            read_tree_pos++;

            if (node->children[0])
            {
                // Branch
                descend(model, read_tree_pos, path | (0 << path_bits), path_bits + 1, node->children[0]);
                model.read_tree[prev_read_tree_pos] = read_tree_pos - prev_read_tree_pos;
                descend(model, read_tree_pos, path | (1 << path_bits), path_bits + 1, node->children[1]);
            }
            else
            {
                // Leaf
                assert(node->value < model.write_list.size());
                assert(path_bits < sizeof(unsigned int) * CHAR_BIT);
                model.read_tree[prev_read_tree_pos] = -node->value;
                model.write_list[node->value] = path | (1 << path_bits);
            }

            delete node;
        }
    };

    class Reader
    {
    public:
        Reader()
        {}

        Reader(const HuffmanModel &model)
            : ptr(model.read_tree.data())
        {}

        bool needs_bit() const
        {
            return *ptr > 0;
        }

        unsigned int get_result() const
        {
            assert(!needs_bit());
            return -*ptr;
        }

        void recv_bit(bool bit)
        {
            assert(needs_bit());
            ptr += bit ? *ptr : 1;
        }

    private:
        const signed int *ptr;
    };

    class Writer
    {
    public:
        Writer()
        {}

        Writer(const HuffmanModel<outputs> &model, unsigned int value)
            : path(model.write_list[value])
        {}

        bool has_bit() const
        {
            assert(path >= 1);
            return path != 1;
        }

        bool get_bit() const
        {
            assert(has_bit());
            return path & 1;
        }

        void next_bit()
        {
            assert(has_bit());
            path >>= 1;
        }

        unsigned int bits_left() const
        {
            return FastMath::log2(path);
        }
        unsigned int get_remaining_bits() const
        {
            return path;
        }

    private:
        unsigned int path;
    };

private:
    std::array<signed int, outputs * 2 - 1> read_tree;
    std::array<unsigned int, outputs> write_list;
};

}

#endif // HUFFMANMODEL_H
