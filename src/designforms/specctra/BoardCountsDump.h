#pragma once

#include <string>

#include "designforms/specctra/BoardReadResult.h"

namespace freerouting::designforms::specctra {

/**
 * Renders the board state of a DSN read result as JSON in the schema of the Java oracle harness
 * tools/java_ref/BoardCounts.java (T007): layers, counts, clearance classes and matrix, the item
 * list in item-list order (descending id) and the warning count. Used by
 * `freerouting --dump-board-counts <dsn> [out.json]`.
 */
class BoardCountsDump
{
public:
    static std::string render(const BoardReadResult& p_result, const std::string& p_dsn_path, bool p_omit_items);

    /** Java BoardCounts.jsonString: a quoted, escaped JSON string ("null" is not produced here). */
    static std::string json_string(const std::string& p_text);
};

} // namespace freerouting::designforms::specctra
