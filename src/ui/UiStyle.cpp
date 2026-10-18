#include "imgui.h"

#include "ui/UiStyle.h"

// Global ImGui theme: dark panel background with a sky-blue accent.
// Runs between rlImGuiBeginInitImGui and rlImGuiEndInitImGui so the custom
// font joins the atlas before the backend uploads it.
static ImFont *titleFont = nullptr;

ImFont *uiTitleFont() {
    return titleFont;
}

void applyUiStyle() {
    ImGui::StyleColorsDark();

    ImGuiStyle &s = ImGui::GetStyle();
    s.WindowRounding = 0.0f;
    s.FrameRounding = 4.0f;
    s.GrabRounding = 4.0f;
    s.FramePadding = ImVec2(8.0f, 6.0f);
    s.ItemSpacing = ImVec2(8.0f, 6.0f);

    const ImVec4 ink = ImVec4(0.96f, 0.96f, 0.96f, 1.0f); // RAYWHITE text
    const ImVec4 faint = ImVec4(0.51f, 0.51f, 0.51f, 1.0f); // GRAY
    const ImVec4 row = ImVec4(0.31f, 0.31f, 0.31f, 1.0f); // DARKGRAY
    const ImVec4 hover = ImVec4(0.51f, 0.51f, 0.51f, 1.0f); // GRAY
    const ImVec4 accent = ImVec4(0.40f, 0.75f, 1.00f, 1.0f); // SKYBLUE
    const ImVec4 panel = ImVec4(0.25f, 0.25f, 0.25f, 1.0f); // dialog grey
    const ImVec4 recess = ImVec4(0.13f, 0.13f, 0.13f, 1.0f); // list recess

    ImVec4 *c = s.Colors;
    c[ImGuiCol_Text] = ink;
    c[ImGuiCol_TextDisabled] = faint;
    c[ImGuiCol_WindowBg] = panel;
    c[ImGuiCol_FrameBg] = recess;
    c[ImGuiCol_FrameBgHovered] = hover;
    c[ImGuiCol_FrameBgActive] = accent;
    c[ImGuiCol_Button] = row;
    c[ImGuiCol_ButtonHovered] = hover;
    c[ImGuiCol_ButtonActive] = accent;
    c[ImGuiCol_CheckMark] = accent;
    c[ImGuiCol_Header] = row;
    c[ImGuiCol_HeaderHovered] = hover;
    c[ImGuiCol_HeaderActive] = accent;
    c[ImGuiCol_SliderGrab] = accent;
    c[ImGuiCol_SliderGrabActive] = accent;

    // Keep pressed-button and selected-row text readable on the accent fill.
    c[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.6f);

    ImGuiIO &io = ImGui::GetIO();
    if (io.Fonts->AddFontFromFileTTF("fonts/DejaVuSans.ttf", 20.0f) != nullptr) {
        io.FontDefault = io.Fonts->Fonts.back();
        titleFont = io.Fonts->AddFontFromFileTTF("fonts/DejaVuSans.ttf", 28.0f);
    }
}
