#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace freerouting::datastructures {

/**
 * Database of objects for which Undo and Redo operations are made possible.
 * The algorithm works only for objects containing no references.
 * Mirrors Java UndoableObjects.java (343 lines).
 */
class UndoableObjects {
public:
    class Storable {
    public:
        virtual ~Storable() = default;
        /// Creates an exact copy of this object (Java Object.clone()).
        virtual std::unique_ptr<Storable> clone() const = 0;
        virtual bool operator<(const Storable& other) const = 0;
    };

    /**
     * Stores information for correctly restoring or cancelling an object in an
     * undo or redo operation. level is the level in the undo stack where this
     * object was inserted. Mirrors Java UndoableObjectNode.
     */
    struct UndoableObjectNode {
        std::unique_ptr<Storable> object;
        int level;
        std::shared_ptr<UndoableObjectNode> undo_object; // state to restore on undo
        std::shared_ptr<UndoableObjectNode> redo_object; // state to restore on redo

        UndoableObjectNode(std::unique_ptr<Storable> p_object, int p_level)
            : object(std::move(p_object)), level(p_level), undo_object(nullptr),
              redo_object(nullptr) {}
    };

    UndoableObjects();
    ~UndoableObjects() = default;

    /// Returns an iterator for sequential reading of the object list.
    class Iterator {
    public:
        explicit Iterator(std::map<Storable*, std::shared_ptr<UndoableObjectNode>>::iterator p_it)
            : it_(p_it) {}
        Iterator& operator++() { ++it_; return *this; }

    private:
        friend class UndoableObjects;
        std::map<Storable*, std::shared_ptr<UndoableObjectNode>>::iterator it_;
    };

    Iterator start_read_object();

    /// Reads the next object in this list. Returns nullptr if the list is exhausted.
    Storable* read_object(Iterator& p_it);

    /// Adds object to the UndoableObjectsList.
    void insert(std::unique_ptr<Storable> p_object);

    /// Removes object from the top level of the UndoableObjectsList.
    /// Returns false if object was not found in the list.
    bool delete_object(Storable* p_object);

    /// Makes the current state of the list restorable by undo.
    void generate_snapshot();

    /**
     * Restores the situation before the last snapshot. Outputs the cancelled
     * and the restored objects (if != nullptr) to enable the calling function
     * to take additional actions needed for these objects. Returns false if no
     * more undo is possible.
     */
    bool undo(std::vector<Storable*>* p_cancelled_objects,
              std::vector<Storable*>* p_restored_objects);

    /**
     * Restores the situation before the last undo. Outputs the cancelled and
     * the restored objects (if != nullptr). Returns false if no more redo is
     * possible.
     */
    bool redo(std::vector<Storable*>* p_cancelled_objects,
              std::vector<Storable*>* p_restored_objects);

    /**
     * Removes the top snapshot from the undo stack, so that its situation
     * cannot be restored anymore. Returns false if no snapshot could be popped.
     */
    bool pop_snapshot();

    /// Must be called before an object is modified after a snapshot for the
    /// first time, if it may have existed before that snapshot.
    void save_for_undo(Storable* p_object);

    int stack_level() const { return stack_level_; }

    size_t object_count() const { return objects_.size(); }

private:
    void disable_redo();

    struct StorablePtrCompare {
        bool operator()(const Storable* lhs, const Storable* rhs) const {
            if (lhs == nullptr || rhs == nullptr) {
                return lhs < rhs;
            }
            return *lhs < *rhs;
        }
    };

    std::map<Storable*, std::shared_ptr<UndoableObjectNode>, StorablePtrCompare> objects_;
    std::vector<std::vector<std::shared_ptr<UndoableObjectNode>>> deleted_objects_stack_;
    int stack_level_ = 0;
    bool redo_possible_ = false;
    mutable std::recursive_mutex mutex_;
};

} // namespace freerouting::datastructures