#pragma once

#include "editor/editor_state.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace ai3
{
inline constexpr std::string_view scene_document_extension = ".ai3scene";

bool serialize_scene_document(const EditorState& scene, std::string& document,
                              std::string* error = nullptr);
bool deserialize_scene_document(std::string_view document, EditorState& destination,
                                std::string* error = nullptr);
bool save_scene_document_file(const EditorState& scene, const std::filesystem::path& path,
                              std::string* error = nullptr);
bool load_scene_document_file(const std::filesystem::path& path, EditorState& destination,
                              std::string* error = nullptr);
} // namespace ai3
