// Tests for the library scope reader (Java parser.Library). The expected padstack dump was printed
// by a throwaway Java program reading the same DSN through Java DsnReader (2.1.0 jar).

#include "designforms/specctra/parser/Library.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "board/BoardLibrary.h"
#include "board/Padstack.h"
#include "board/Padstacks.h"
#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/ReadScopeParameter.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"
#include "geometry/planar/ConvexTileShape.h"
#include "geometry/planar/IntBox.h"
#include "geometry/planar/IntOctagon.h"
#include "geometry/planar/Simplex.h"

namespace freerouting::designforms::specctra::parser {
namespace {

namespace fp = geometry::planar;

std::string shape_class(const fp::Shape& p_shape)
{
    if (dynamic_cast<const fp::Circle*>(&p_shape) != nullptr)
    {
        return "Circle";
    }
    if (const auto* tile = dynamic_cast<const fp::ConvexTileShape*>(&p_shape))
    {
        if (dynamic_cast<const fp::IntBox*>(&tile->tile()) != nullptr)
        {
            return "IntBox";
        }
        if (dynamic_cast<const fp::IntOctagon*>(&tile->tile()) != nullptr)
        {
            return "IntOctagon";
        }
        if (dynamic_cast<const fp::Simplex*>(&tile->tile()) != nullptr)
        {
            return "Simplex";
        }
    }
    return "?";
}

std::string dump_padstacks(const board::RoutingBoard& p_board)
{
    std::ostringstream out;
    const board::Padstacks& padstacks = p_board.library().padstacks();
    for (int i = 1; i <= padstacks.count(); ++i)
    {
        const board::Padstack* p = padstacks.get(i);
        out << "PAD " << p->get_name() << " attach=" << (p->is_attach_allowed() ? "true" : "false")
            << " abs=" << (p->is_placed_absolute() ? "true" : "false") << " from=" << p->get_from_layer()
            << " to=" << p->get_to_layer() << ":";
        for (int l = 0; l < p_board.get_layer_count(); ++l)
        {
            const fp::Shape* shape = p->get_shape(l);
            if (shape == nullptr)
            {
                out << " [null]";
                continue;
            }
            const fp::IntBox box = shape->bounding_box();
            out << " [" << shape_class(*shape) << " " << box.ll.x << "," << box.ll.y << "," << box.ur.x << ","
                << box.ur.y << "]";
        }
        out << "\n";
    }
    return out.str();
}

constexpr const char* kLib1 =
    "(resolution um 10)"
    "(structure (layer F.Cu (type signal)) (layer B.Cu (type signal)) (boundary (rect pcb 0 0 100000 100000)))"
    "(library"
    "  (padstack Round.5 (shape (circle signal 600)) (attach off))"
    "  (padstack RectTop (shape (rect F.Cu -500 -300 500 300)) (absolute on))"
    "  (padstack Concave (shape (polygon signal 0 0 0 0 1000 0 1000 1000 500 400 0 1000)))"
    "  (padstack Zero (shape (rect F.Cu 0 0 0 0)))"
    "  (padstack RectTop (shape (rect B.Cu -1 -1 1 1)))"
    "  (padstack NoShape)"
    "  (padstack TwoLayers (shape (rect F.Cu -200 -200 200 200)) (shape (circle B.Cu 300 100 50)) (unknown (nested 1)))"
    "))";

TEST(LibraryTest, PadstacksMatchJava)
{
    SpecctraDsnStreamReader scanner(kLib1, true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    EXPECT_EQ("PAD Round attach=false abs=false from=0 to=1: [Circle -3000,-3000,3000,3000] [Circle -3000,-3000,3000,3000]\n"
              "PAD RectTop attach=true abs=true from=0 to=0: [IntBox -5000,-3000,5000,3000] [null]\n"
              "PAD Concave attach=true abs=false from=0 to=1: [Simplex 0,0,10000,10000] [Simplex 0,0,10000,10000]\n"
              "PAD Zero attach=true abs=false from=0 to=0: [IntBox -1,-1,1,1] [null]\n"
              "PAD TwoLayers attach=true abs=false from=0 to=1: [IntBox -2000,-2000,2000,2000] [Circle -500,-1000,2500,2000]\n",
        dump_padstacks(*par.get_board()));
}

TEST(LibraryTest, LibraryWithoutBoardFails)
{
    SpecctraDsnStreamReader scanner("(padstack P (shape (circle signal 10))))", true);
    ReadScopeParameter par(scanner);
    EXPECT_FALSE(Keyword::LIBRARY_SCOPE->read_scope(par));
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
