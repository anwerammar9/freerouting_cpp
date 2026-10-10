// Tests for the library scope reader (Java parser.Library). The expected padstack dump was printed
// by a throwaway Java program reading the same DSN through Java DsnReader (2.1.0 jar).

#include "designforms/specctra/parser/Library.h"

#include <gtest/gtest.h>

#include <sstream>
#include <set>
#include <string>

#include "board/BoardLibrary.h"
#include "board/Package.h"
#include "board/Padstack.h"
#include "board/Padstacks.h"
#include "board/RoutingBoard.h"
#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/PartLibrary.h"
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

std::string format_double(double p_value)
{
    std::ostringstream out;
    out << p_value;
    std::string text = out.str();
    if (text.find('.') == std::string::npos)
    {
        text += ".0"; // Java prints doubles with a fraction part
    }
    return text;
}

std::string format_box(const fp::IntBox& p_box)
{
    std::ostringstream out;
    out << "IntBox(ll=(" << p_box.ll.x << "," << p_box.ll.y << "), ur=(" << p_box.ur.x << "," << p_box.ur.y << "))";
    return out.str();
}

std::string dump_packages(const board::RoutingBoard& p_board)
{
    std::ostringstream out;
    const board::Packages& packages = p_board.library().packages();
    for (int i = 1; i <= packages.count(); ++i)
    {
        const board::Package* p = packages.get(i);
        out << "PKG " << p->get_name() << " front=" << (p->is_front() ? "true" : "false") << " pins:";
        for (int k = 0; k < p->pin_count(); ++k)
        {
            const board::Package::Pin* pin = p->get_pin(k);
            const fp::FloatPoint f = pin->relative_location.to_float();
            out << " " << pin->name << "/" << pin->padstack_id << "/" << static_cast<int>(f.x) << ","
                << static_cast<int>(f.y) << "/" << format_double(pin->rotation_in_degree);
        }
        out << " outlines=" << p->outline().size() << " widths=[";
        for (size_t k = 0; k < p->outline_widths().size(); ++k)
        {
            out << (k > 0 ? ", " : "") << format_double(p->outline_widths()[k]);
        }
        out << "] closed=[";
        for (size_t k = 0; k < p->outline_is_closed().size(); ++k)
        {
            out << (k > 0 ? ", " : "") << (p->outline_is_closed()[k] ? "true" : "false");
        }
        out << "] keepouts:";
        for (const auto& k : p->keepouts())
        {
            out << " " << k.name << "@" << k.layer << ":" << format_box(k.area->bounding_box());
        }
        out << " via:";
        for (const auto& k : p->via_keepouts())
        {
            out << " " << k.name << "@" << k.layer << ":" << format_box(k.area->bounding_box());
        }
        out << "\n";
    }
    return out.str();
}

constexpr const char* kLib2 =
    "(resolution um 10)"
    "(structure (layer F.Cu (type signal)) (layer B.Cu (type signal)) (boundary (rect pcb 0 0 100000 100000)))"
    "(library"
    "  (image R0805"
    "    (outline (path signal 120 -1000 -500 1000 -500 1000 500 -1000 500 -1000 -500))"
    "    (outline (path signal 0 -900 0 900 0))"
    "    (pin Rect 1 -950 0) (pin Rect 2 950 0 (rotate 90))"
    "    (keepout (rect F.Cu -100 -100 100 100)) (keepout named (rect B.Cu -50 -50 50 50)))"
    "  (image R0805 (pin Rect 1 -950 0) (pin Rect 2 950 0 (rotate 90)))"
    "  (image R0805 (pin Rect 1 -900 0))"
    "  (image BackPart (side back) (pin Round A1 0 0) (via_keepout (circle signal 200)))"
    "  (image Rot (pin Rect (rotate 45) 7 100 200))"
    "  (padstack Rect (shape (rect signal -300 -200 300 200)))"
    "  (padstack Round (shape (circle signal 400)))"
    "))";

TEST(LibraryTest, PackagesMatchJava)
{
    SpecctraDsnStreamReader scanner(kLib2, true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PCB_SCOPE->read_scope(par));
    ASSERT_NE(nullptr, par.get_board());
    // identical second R0805 is dropped, the different third one becomes R0805::1; one unnamed keepout
    // renames all keepouts of the image (Java generateMissingKeepoutNames)
    EXPECT_EQ("PKG R0805 front=true pins: 1/1/-9500,0/0.0 2/1/9500,0/90.0 outlines=2 widths=[120.0, 0.0] "
              "closed=[true, false] keepouts: keepout_1@0:IntBox(ll=(-1000,-1000), ur=(1000,1000)) "
              "keepout_2@1:IntBox(ll=(-500,-500), ur=(500,500)) via:\n"
              "PKG R0805::1 front=true pins: 1/1/-9000,0/0.0 outlines=0 widths=[] closed=[] keepouts: via:\n"
              "PKG BackPart front=false pins: A1/2/0,0/0.0 outlines=0 widths=[] closed=[] keepouts: via: "
              "via_keepout_1@-1:IntBox(ll=(-1000,-1000), ur=(1000,1000))\n"
              "PKG Rot front=true pins: 7/1/1000,2000/45.0 outlines=0 widths=[] closed=[] keepouts: via:\n",
        dump_packages(*par.get_board()));
}

TEST(LibraryTest, PartLibraryIsReadIntoTheScopeParameter)
{
    SpecctraDsnStreamReader scanner(
        "(logical_part_mapping LM358 (comp U2 U1))"
        "(logical_part LM358 (pin 1 1 A 0 OUT 0) (pin 2 2 A 0 IN- 1 (subgate x)) (unknown 1)))",
        true);
    ReadScopeParameter par(scanner);
    ASSERT_TRUE(Keyword::PART_LIBRARY_SCOPE->read_scope(par));
    ASSERT_EQ(1u, par.logical_part_mappings.size());
    EXPECT_EQ("LM358", par.logical_part_mappings[0].name);
    EXPECT_EQ((std::set<std::string>{"U1", "U2"}), par.logical_part_mappings[0].components); // sorted
    ASSERT_EQ(1u, par.logical_parts.size());
    ASSERT_EQ(2u, par.logical_parts[0].part_pins.size());
    const PartLibrary::PartPin& pin = par.logical_parts[0].part_pins[1];
    EXPECT_EQ("2", pin.pin_name);
    EXPECT_EQ("A", pin.gate_name);
    EXPECT_EQ(0, pin.gate_swap_code);
    EXPECT_EQ("IN-", pin.gate_pin_name);
    EXPECT_EQ(1, pin.gate_pin_swap_code);
}

} // namespace
} // namespace freerouting::designforms::specctra::parser
