#include "interactive/HeadlessBoardManager.h"

#include <cstddef>

namespace freerouting {
namespace interactive {

HeadlessBoardManager::HeadlessBoardManager()
{
}

HeadlessBoardManager::~HeadlessBoardManager()
{
}

void HeadlessBoardManager::open_board(const std::string& filepath)
{
    // TODO: Implement board loading
}

void HeadlessBoardManager::start_interface(std::weak_ptr<BoardManager> p_observer)
{
    // Stub - no GUI interface in headless mode
}

void HeadlessBoardManager::refresh()
{
}

void HeadlessBoardManager::zoom_fit()
{
}

} // namespace interactive
} // namespace freerouting