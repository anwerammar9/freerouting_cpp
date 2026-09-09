#pragma once

#include <memory>
#include <vector>
#include <map>
#include <list>
#include <iterator>
#include <mutex>

namespace freerouting::datastructures {

class UndoableObjects {
public:
    class Storable {
    public:
        virtual ~Storable() = default;
        virtual std::unique_ptr<Storable> clone() const = 0;
        virtual bool operator<(const Storable& other) const = 0;
    };

    struct UndoableObjectNode {
        std::unique_ptr<Storable> object;
        int level;
        std::unique_ptr<UndoableObjectNode> undo_object;
        UndoableObjectNode* redo_object = nullptr; // Raw pointer to avoid circular dependency in unique_ptr

        UndoableObjectNode(std::unique_ptr<Storable> p_object, int p_level)
            : object(std::move(p_object)), level(p_level), undo_object(nullptr), redo_object(nullptr) {}
    };

    UndoableObjects();
    ~UndoableObjects();

    // Iterator for sequential reading
    class Iterator {
    public:
        using MapIterator = std::map<Storable*, std::unique_ptr<UndoableObjectNode>>::iterator;
        
        Iterator(MapIterator it) : it_(it) {}
        MapIterator it_;
    };

    Iterator start_read_object();
    Storable* read_object(Iterator& p_it);

    void insert(std::unique_ptr<Storable> p_object);
    bool delete_object(Storable* p_object);

    void generate_snapshot();
    bool undo(std::vector<std::unique_ptr<Storable>>* p_cancelled_objects,
              std::vector<std::unique_ptr<Storable>>* p_restored_objects);
    bool redo(std::vector<std::unique_ptr<Storable>>* p_cancelled_objects,
              std::vector<std::vector<std::unique_ptr<Storable>>*> p_restored_objects);

    void save_for_undo(Storable* p_object);

private:
    void disable_redo();

    struct StorablePtrCompare {
        bool operator()(const Storable* lhs, const Storable* rhs) const {
            if (!lhs || !rhs) return lhs < rhs;
            return *lhs < *rhs;
        }
    };

    std::map<Storable*, std::unique_ptr<UndoableObjectNode>, StorablePtrCompare> objects;
    std::vector<std::list<UndoableObjectNode*>> deleted_objects_stack;
    int stack_level;
    bool redo_possible;
    mutable std::recursive_mutex mutex_;
};

} // namespace freerouting::datastructures
