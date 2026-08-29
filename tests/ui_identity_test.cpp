#include <doctest/doctest.h>

#include "ui/ui_identity.h"

TEST_CASE("localized ImGui labels retain stable hidden identities")
{
    const std::string english = ai3::stable_imgui_label("Scene Graph", "ai3_scene_graph");
    const std::string spanish = ai3::stable_imgui_label("Grafo de escena", "ai3_scene_graph");
    CHECK(english == "Scene Graph###ai3_scene_graph");
    CHECK(spanish == "Grafo de escena###ai3_scene_graph");
    CHECK(english.substr(english.find("###")) == spanish.substr(spanish.find("###")));
}
