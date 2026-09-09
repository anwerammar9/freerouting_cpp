#pragma once

#include <vector>
#include <optional>
#include <stdexcept>

namespace freerouting {
namespace datastructures {

/**
 * Implementation of a stack as a vector
 */
template <typename T>
class ArrayStack
{
public:
    explicit ArrayStack(size_t p_max_stack_depth);

    void reset();
    void push(const T& p_element);
    std::optional<T> pop();

private:
    void reallocate();

    int level = -1;
    std::vector<T> node_arr;
};

template <typename T>
ArrayStack<T>::ArrayStack(size_t p_max_stack_depth)
{
    node_arr.resize(p_max_stack_depth);
}

template <typename T>
void ArrayStack<T>::reset()
{
    level = -1;
}

template <typename T>
void ArrayStack<T>::push(const T& p_element)
{
    ++level;

    if (level >= static_cast<int>(node_arr.size()))
    {
        reallocate();
    }

    node_arr[level] = p_element;
}

template <typename T>
std::optional<T> ArrayStack<T>::pop()
{
    if (level < 0)
    {
        return std::nullopt;
    }
    T result = node_arr[level];
    --level;
    return result;
}

template <typename T>
void ArrayStack<T>::reallocate()
{
    size_t new_size = node_arr.size() == 0 ? 4 : node_arr.size() * 4;
    node_arr.resize(new_size);
}

} // namespace datastructures
} // namespace freerouting
