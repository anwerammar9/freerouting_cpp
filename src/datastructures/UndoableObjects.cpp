#include "datastructures/UndoableObjects.h"

#include <algorithm>

namespace freerouting::datastructures {

UndoableObjects::UndoableObjects() = default;

UndoableObjects::Iterator UndoableObjects::start_read_object()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return Iterator(objects_.begin());
}

UndoableObjects::Storable* UndoableObjects::read_object(Iterator& p_it)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    while (p_it.it_ != objects_.end())
    {
        const std::shared_ptr<UndoableObjectNode>& curr_node = p_it.it_->second;
        ++p_it.it_;
        // Skip objects which only become alive again by redo.
        if (curr_node && curr_node->level <= stack_level_)
        {
            return curr_node->object.get();
        }
    }
    return nullptr;
}

void UndoableObjects::insert(std::unique_ptr<Storable> p_object)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    disable_redo();
    Storable* ptr = p_object.get();
    // Java keys the map by the object value; the comparator gives the same
    // value-equivalence semantics for the pointer keys.
    objects_[ptr] = std::make_shared<UndoableObjectNode>(std::move(p_object), stack_level_);
}

bool UndoableObjects::delete_object(Storable* p_object)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    disable_redo();

    std::vector<std::shared_ptr<UndoableObjectNode>>* current_delete_list = nullptr;
    if (!deleted_objects_stack_.empty())
    {
        current_delete_list = &deleted_objects_stack_.back();
    }

    auto it = objects_.find(p_object);
    if (it == objects_.end())
    {
        return false;
    }

    const std::shared_ptr<UndoableObjectNode>& object_node = it->second;
    if (current_delete_list != nullptr)
    {
        if (object_node->level < stack_level_)
        {
            // Add the current node to the current delete list to make undo possible.
            current_delete_list->push_back(object_node);
        }
        else if (object_node->undo_object != nullptr)
        {
            // Add the node's undo object to the current delete list to make undo possible.
            current_delete_list->push_back(object_node->undo_object);
        }
    }

    objects_.erase(it);
    return true;
}

void UndoableObjects::generate_snapshot()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    disable_redo();
    deleted_objects_stack_.emplace_back();
    ++stack_level_;
}

bool UndoableObjects::undo(std::vector<Storable*>* p_cancelled_objects,
                           std::vector<Storable*>* p_restored_objects)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (stack_level_ == 0)
    {
        return false; // no more undo possible
    }
    for (auto it = objects_.begin(); it != objects_.end(); ++it)
    {
        // Local copy: Java iterates over the map values and the map mutation
        // below must not change what this loop observes.
        const std::shared_ptr<UndoableObjectNode> curr_node = it->second;
        if (curr_node->level == stack_level_)
        {
            if (curr_node->undo_object != nullptr)
            {
                // Replace the current object by its previous state.
                const std::shared_ptr<UndoableObjectNode> restore_node = curr_node->undo_object;
                restore_node->redo_object = curr_node;
                it->second = restore_node;
                if (p_restored_objects != nullptr)
                {
                    p_restored_objects->push_back(restore_node->object.get());
                }
            }
            if (p_cancelled_objects != nullptr)
            {
                p_cancelled_objects->push_back(curr_node->object.get());
            }
        }
    }
    // Restore the deleted objects.
    std::vector<std::shared_ptr<UndoableObjectNode>>& current_delete_list =
        deleted_objects_stack_.at(static_cast<size_t>(stack_level_ - 1));
    for (const std::shared_ptr<UndoableObjectNode>& curr_deleted_node : current_delete_list)
    {
        objects_[curr_deleted_node->object.get()] = curr_deleted_node;
        if (p_restored_objects != nullptr)
        {
            p_restored_objects->push_back(curr_deleted_node->object.get());
        }
    }
    --stack_level_;
    redo_possible_ = true;
    return true;
}

bool UndoableObjects::redo(std::vector<Storable*>* p_cancelled_objects,
                           std::vector<Storable*>* p_restored_objects)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (stack_level_ >= static_cast<int>(deleted_objects_stack_.size()))
    {
        return false; // already at the top level
    }
    ++stack_level_;
    for (auto it = objects_.begin(); it != objects_.end(); ++it)
    {
        const std::shared_ptr<UndoableObjectNode> curr_node = it->second;
        if (curr_node->redo_object != nullptr && curr_node->redo_object->level == stack_level_)
        {
            // Object was created on a lower level and changed on the current
            // level: replace the lower level object by the object on the
            // current level.
            const std::shared_ptr<UndoableObjectNode> restore_node = curr_node->redo_object;
            it->second = restore_node;
            if (p_cancelled_objects != nullptr)
            {
                p_cancelled_objects->push_back(curr_node->object.get());
            }
            if (p_restored_objects != nullptr)
            {
                p_restored_objects->push_back(restore_node->object.get());
            }
        }
        else if (curr_node->level == stack_level_)
        {
            // Object was created on the current level; allow it to be restored.
            if (p_restored_objects != nullptr)
            {
                p_restored_objects->push_back(curr_node->object.get());
            }
        }
    }
    // Delete the objects which were deleted on the current level, again.
    std::vector<std::shared_ptr<UndoableObjectNode>>& current_delete_list =
        deleted_objects_stack_.at(static_cast<size_t>(stack_level_ - 1));
    for (std::shared_ptr<UndoableObjectNode> curr_deleted_node : current_delete_list)
    {
        while (curr_deleted_node->redo_object != nullptr
               && curr_deleted_node->redo_object->level <= stack_level_)
        {
            curr_deleted_node = curr_deleted_node->redo_object;
        }
        if (objects_.erase(curr_deleted_node->object.get()) == 0)
        {
            // Mirrors Java FRLogger.warn("previous deleted object not found").
        }
        bool already_restored = false;
        if (p_restored_objects != nullptr)
        {
            auto it = std::find(p_restored_objects->begin(), p_restored_objects->end(),
                                curr_deleted_node->object.get());
            if (it != p_restored_objects->end())
            {
                p_restored_objects->erase(it);
                already_restored = true;
            }
        }
        if (!already_restored && p_cancelled_objects != nullptr)
        {
            // The object only needs to be cancelled if it is already in the board.
            p_cancelled_objects->push_back(curr_deleted_node->object.get());
        }
    }
    return true;
}

bool UndoableObjects::pop_snapshot()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    disable_redo();
    if (stack_level_ == 0)
    {
        return false;
    }
    for (auto it = objects_.begin(); it != objects_.end(); ++it)
    {
        const std::shared_ptr<UndoableObjectNode>& curr_node = it->second;
        if (curr_node->level == stack_level_ - 1)
        {
            if (curr_node->redo_object != nullptr && curr_node->redo_object->level == stack_level_)
            {
                curr_node->redo_object->undo_object = curr_node->undo_object;
                if (curr_node->undo_object != nullptr)
                {
                    curr_node->undo_object->redo_object = curr_node->redo_object;
                }
            }
        }
        else if (curr_node->level >= stack_level_)
        {
            --curr_node->level;
        }
    }
    size_t deleted_objects_stack_size = deleted_objects_stack_.size();
    if (deleted_objects_stack_size >= 2)
    {
        // Join the top delete list with the delete list of the second top level.
        std::vector<std::shared_ptr<UndoableObjectNode>>& from_delete_list =
            deleted_objects_stack_.back();
        std::vector<std::shared_ptr<UndoableObjectNode>>& to_delete_list =
            deleted_objects_stack_[deleted_objects_stack_size - 2];
        for (const std::shared_ptr<UndoableObjectNode>& curr_deleted_node : from_delete_list)
        {
            if (curr_deleted_node->level < stack_level_ - 1)
            {
                to_delete_list.push_back(curr_deleted_node);
            }
            else if (curr_deleted_node->undo_object != nullptr)
            {
                to_delete_list.push_back(curr_deleted_node->undo_object);
            }
        }
    }
    deleted_objects_stack_.pop_back();
    --stack_level_;
    return true;
}

void UndoableObjects::save_for_undo(Storable* p_object)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    disable_redo();
    auto it = objects_.find(p_object);
    if (it == objects_.end())
    {
        return;
    }
    const std::shared_ptr<UndoableObjectNode>& curr_node = it->second;
    if (curr_node->level < stack_level_)
    {
        auto old_node = std::make_shared<UndoableObjectNode>(p_object->clone(), curr_node->level);
        old_node->undo_object = curr_node->undo_object;
        old_node->redo_object = curr_node;
        curr_node->undo_object = old_node;
        curr_node->level = stack_level_;
    }
}

void UndoableObjects::disable_redo()
{
    if (!redo_possible_)
    {
        return;
    }
    redo_possible_ = false;
    // Shorten the deleted objects stack to the current level.
    if (static_cast<int>(deleted_objects_stack_.size()) > stack_level_)
    {
        deleted_objects_stack_.erase(deleted_objects_stack_.begin() + stack_level_,
                                     deleted_objects_stack_.end());
    }
    for (auto it = objects_.begin(); it != objects_.end();)
    {
        const std::shared_ptr<UndoableObjectNode>& curr_node = it->second;
        if (curr_node->level > stack_level_)
        {
            it = objects_.erase(it);
        }
        else
        {
            if (curr_node->level == stack_level_)
            {
                curr_node->redo_object.reset();
            }
            ++it;
        }
    }
}

} // namespace freerouting::datastructures