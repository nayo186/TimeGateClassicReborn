///////////////////////////////////////////////////////////////////////////////
// Time Gate: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Time Gate's launcher screen, in the colours of its cover (menuBase.h's
// palette of this edition): the cover blurred behind the whole window, the
// cover itself, the title, a big Play button, three cards (Options, Controls,
// Audio), the default settings and, in a bar at the bottom, the data folder,
// Browse, Update (only when there is a newer version), About and Exit. Only
// layout and drawing, like launcherScreen.h, whose LauncherView and
// LauncherIntent it shares. Two layouts: landscape and, in a narrow window or
// on a phone, one column that scrolls.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "launcherScreen.h"

#ifdef CLASSIC_REBORN_EDITION_TIMEGATE

namespace timeGateScreen
{

// The size, in interface units, each layout was drawn for
static const ImVec2 kDesignLandscape(960.0f, 600.0f);
static const ImVec2 kDesignPortrait(480.0f, 900.0f);

// The ALPHA / BETA mark: the blue of the knight's armour
static const ImU32 kBadge = IM_COL32(110, 132, 230, 255);
static const ImU32 kBadgeText = IM_COL32(196, 206, 255, 255);

// A picture filling its frame (cropped, never stretched), rounded
inline void fillImage(ImDrawList* draw, ImTextureID picture, int width, int height, ImVec2 min, ImVec2 max, float rounding)
{
    if (!picture || width <= 0 || height <= 0)
        return;
    const float frameW = max.x - min.x, frameH = max.y - min.y;
    const float scale = (std::max)(frameW / width, frameH / height);
    const float u = frameW / (width * scale), v = frameH / (height * scale);
    draw->AddImageRounded(picture, min, max, ImVec2((1.0f - u) * 0.5f, (1.0f - v) * 0.5f),
        ImVec2((1.0f + u) * 0.5f, (1.0f + v) * 0.5f), IM_COL32_WHITE, rounding);
}

// "ALPHA" or "BETA" from the version (0.1.0-alpha.1), empty for a final one
inline const char* versionStage(const std::string& version)
{
    if (version.find("alpha") != std::string::npos)
        return "ALPHA";
    if (version.find("beta") != std::string::npos)
        return "BETA";
    return "";
}

struct Parts
{
    const LauncherView& view;
    const menu::Metrics& m;
    const menu::Fonts& fonts;
    LauncherIntent& intent;
    ImDrawList* draw;
    bool linked;

    // Size in pixels of `units` of the screen design
    float u(float units) const { return m.px(units); }

    // A clickable area drawn by hand; true when pressed
    bool area(const char* id, ImVec2 pos, ImVec2 size, bool enabled, bool& hovered)
    {
        ImGui::SetCursorScreenPos(pos);
        ImGui::BeginDisabled(!enabled);
        const bool clicked = ImGui::InvisibleButton(id, size);
        ImGui::EndDisabled();
        hovered = enabled && (ImGui::IsItemHovered() || ImGui::IsItemFocused());
        return clicked && enabled;
    }

    // "TIME GATE Classic Reborn" on the left, the interface language on the right
    void header(float left, float right, float y)
    {
        ImFont* brand = fonts.button;
        draw->AddText(brand, brand->FontSize, ImVec2(left, y), menu::kGold, "TIME GATE");
        const float brandW = brand->CalcTextSizeA(brand->FontSize, FLT_MAX, 0.0f, "TIME GATE").x;
        draw->AddText(fonts.text, fonts.text->FontSize, ImVec2(left + brandW + u(8.0f), y + (brand->FontSize - fonts.text->FontSize) * 0.5f),
            menu::kTextDim, "Classic Reborn");

        const float comboW = u(124.0f);
        const float comboY = y + (brand->FontSize - ImGui::GetFrameHeight()) * 0.5f;
        const char* label = tr(UiText::InterfaceLanguage);
        const float labelW = ImGui::CalcTextSize(label).x;
        ImGui::SetCursorScreenPos(ImVec2(right - comboW - ImGui::GetStyle().ItemSpacing.x - labelW,
            comboY + ImGui::GetStyle().FramePadding.y));
        ImGui::TextColored(menu::color(menu::kTextDim), "%s", label);
        const int chosen = launcherScreen::comboAt("##uilang", ImVec2(right - comboW, comboY), comboW, uiLanguageName(uiLanguage()),
            launcherScreen::uiLanguageNames(), (int)uiLanguage(), true);
        if (chosen >= 0)
            intent.uiLanguage = chosen;
    }

    // The cover with a soft shadow and a thin orange edge
    void cover(ImVec2 min, ImVec2 max)
    {
        const float rounding = u(10.0f);
        for (int i = 4; i >= 1; i--)
        {
            const float grow = u(3.0f * i);
            draw->AddRectFilled(ImVec2(min.x - grow * 0.6f, min.y - grow * 0.3f + u(6.0f)), ImVec2(max.x + grow * 0.6f, max.y + grow + u(6.0f)),
                IM_COL32(0, 0, 0, 34), rounding + grow);
        }
        draw->AddRectFilled(min, max, menu::kPanel, rounding);
        if (view.cover)
            fillImage(draw, linked ? view.cover->colour : view.cover->grey, view.cover->width, view.cover->height, min, max, rounding);
        draw->AddRect(min, max, (menu::kGold & 0x00FFFFFF) | (linked ? 150u << 24 : 70u << 24), rounding, 0, 1.0f);
    }

    // "Knight's Chase", "Infogrames · 1996" and the ALPHA / BETA mark; returns the y after them
    float title(float x, float y, float width, bool centred)
    {
        const char* name = "Knight's Chase";
        if (centred)
            launcherScreen::fittedCentredText(draw, fonts.title, x + width * 0.5f, y, width, menu::kText, name);
        else
            draw->AddText(fonts.title, fonts.title->FontSize, ImVec2(x, y), menu::kText, name);
        y += fonts.title->FontSize + u(8.0f);
        const char* studio = "Infogrames \xC2\xB7 1996";
        if (centred)
            menu::textCentered(draw, fonts.text, x + width * 0.5f, y, menu::kTextDim, studio);
        else
            draw->AddText(fonts.text, fonts.text->FontSize, ImVec2(x, y), menu::kTextDim, studio);
        y += fonts.text->FontSize + u(10.0f);
        const char* stage = versionStage(view.version);
        if (stage[0])
        {
            ImFont* font = fonts.smallText;
            const ImVec2 textSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, stage);
            const ImVec2 size(textSize.x + u(20.0f), textSize.y + u(6.0f));
            const ImVec2 pos(centred ? x + (width - size.x) * 0.5f : x, y);
            draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), (kBadge & 0x00FFFFFF) | (60u << 24), size.y * 0.5f);
            draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), kBadge, size.y * 0.5f);
            draw->AddText(font, font->FontSize, ImVec2(pos.x + u(10.0f), pos.y + u(3.0f)), kBadgeText, stage);
            y += size.y + u(4.0f);
        }
        return y;
    }

    // The big orange Play button with its triangle
    void playButton(ImVec2 pos, ImVec2 size)
    {
        bool hovered = false;
        if (area("##play", pos, size, linked, hovered))
            intent.play = true;
        if (!linked)
            launcherScreen::tooltipWhenHovered(tr(UiText::DataFolderMissing));
        const ImVec2 corner(pos.x + size.x, pos.y + size.y);
        const float rounding = u(12.0f);
        if (linked)
        {
            draw->AddRectFilled(pos, corner, hovered ? menu::kGoldHoverTop : menu::kGold, rounding);
            draw->AddRectFilled(pos, ImVec2(corner.x, pos.y + size.y * 0.5f), IM_COL32(255, 255, 255, hovered ? 26 : 16), rounding,
                ImDrawFlags_RoundCornersTop);
            draw->AddRect(pos, corner, menu::kGoldBottom, rounding, 0, 1.0f);
        }
        else
        {
            draw->AddRectFilled(pos, corner, menu::kButtonDark, rounding);
            draw->AddRect(pos, corner, menu::kPanelBorder, rounding);
        }
        menu::navCursor(pos, corner, rounding);
        ImFont* font = fonts.button;
        const char* label = tr(UiText::PlayShort);
        const float textW = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, label).x;
        const float side = font->FontSize * 0.7f;
        const float gap = u(12.0f);
        const float startX = pos.x + (size.x - (side + gap + textW)) * 0.5f;
        const float midY = pos.y + size.y * 0.5f;
        const ImU32 textColor = linked ? menu::kGoldText : menu::kTextDisabled;
        draw->AddTriangleFilled(ImVec2(startX, midY - side * 0.55f), ImVec2(startX, midY + side * 0.55f),
            ImVec2(startX + side, midY), textColor);
        draw->AddText(font, font->FontSize, ImVec2(startX + side + gap, midY - font->FontSize * 0.5f), textColor, label);
    }

    // A card: its name, a line of what it holds and a rust mark on the left
    bool card(const char* id, const char* name, const char* detail, ImVec2 pos, ImVec2 size)
    {
        bool hovered = false;
        const bool pressed = area(id, pos, size, linked, hovered);
        const ImVec2 corner(pos.x + size.x, pos.y + size.y);
        const float rounding = u(10.0f);
        draw->AddRectFilled(pos, corner, hovered ? menu::kButtonHover : menu::kPanel, rounding);
        draw->AddRect(pos, corner, hovered ? menu::kGold : menu::kPanelBorder, rounding);
        draw->AddRectFilled(pos, ImVec2(pos.x + u(4.0f), corner.y), linked ? menu::kHeader : menu::kPanelBorder, rounding,
            ImDrawFlags_RoundCornersLeft);
        menu::navCursor(pos, corner, rounding);
        const float textX = pos.x + u(16.0f);
        const float nameY = pos.y + (size.y - fonts.button->FontSize - u(4.0f) - fonts.text->FontSize) * 0.5f;
        draw->AddText(fonts.button, fonts.button->FontSize, ImVec2(textX, nameY), linked ? menu::kText : menu::kTextDisabled, name);
        draw->AddText(fonts.text, fonts.text->FontSize, ImVec2(textX, nameY + fonts.button->FontSize + u(4.0f)),
            linked ? menu::kTextDim : menu::kTextDisabled, detail);
        return pressed;
    }

    // Options, Controls and Audio: in a row, or one under another (column)
    float cards(float x, float y, float width, bool column)
    {
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::PushFont(fonts.smallText);
        ImGui::TextColored(menu::color(menu::kTextDim), "%s", tr(UiText::Settings));
        ImGui::PopFont();
        y += fonts.smallText->FontSize + u(8.0f);
        const char* names[3] = { tr(UiText::OptionsCard), tr(UiText::ControlsCard), tr(UiText::AudioCard) };
        const char* details[3] = { tr(UiText::OptionsDetail), tr(UiText::ControlsDetail), tr(UiText::AudioDetail) };
        bool* actions[3] = { &intent.openAdvanced, &intent.openControls, &intent.openAudio };
        const float gap = u(12.0f);
        const float cardH = m.pressHeight(64.0f);
        const float cardW = column ? width : (width - gap * 2.0f) / 3.0f;
        for (int i = 0; i < 3; i++)
        {
            ImGui::PushID(i);
            const ImVec2 pos = column ? ImVec2(x, y + (cardH + gap) * i) : ImVec2(x + (cardW + gap) * i, y);
            if (card("##card", names[i], details[i], pos, ImVec2(cardW, cardH)))
                *actions[i] = true;
            ImGui::PopID();
        }
        return y + (column ? (cardH + gap) * 3.0f - gap : cardH);
    }

    // The game language (when the folder has more than one) and the default settings
    float settingsRow(float x, float y, float width)
    {
        const float rowH = ImGui::GetFrameHeight();
        const bool languages = linked && view.gameLanguages.size() > 1;
        const float gap = u(12.0f);
        float defaultsX = x, defaultsW = width;
        if (languages)
        {
            const float labelW = ImGui::CalcTextSize(tr(UiText::GameLanguage)).x;
            const float comboW = (std::max)(u(110.0f), width * 0.5f - gap - labelW - ImGui::GetStyle().ItemSpacing.x);
            ImGui::SetCursorScreenPos(ImVec2(x, y + ImGui::GetStyle().FramePadding.y));
            ImGui::TextColored(menu::color(menu::kTextDim), "%s", tr(UiText::GameLanguage));
            const float comboX = x + labelW + ImGui::GetStyle().ItemSpacing.x;
            const char* current = (view.gameLanguage >= 0 && view.gameLanguage < (int)view.gameLanguages.size())
                ? view.gameLanguages[view.gameLanguage].c_str() : tr(UiText::NoLanguageFound);
            const int chosen = launcherScreen::comboAt("##gamelang", ImVec2(comboX, y), comboW, current, view.gameLanguages,
                view.gameLanguage, true);
            if (chosen >= 0)
                intent.gameLanguage = chosen;
            defaultsX = comboX + comboW + gap;
            defaultsW = x + width - defaultsX;
        }
        bool hovered = false;
        const bool enabled = linked && !view.isDefault;
        const ImVec2 pos(defaultsX, y), corner(defaultsX + defaultsW, y + m.pressHeight(rowH / m.scale));
        if (area("##defaults", pos, ImVec2(defaultsW, corner.y - y), enabled, hovered))
            intent.restoreDefaults = true;
        draw->AddRectFilled(pos, corner, hovered ? menu::kButtonHover : (menu::kPanel & 0x00FFFFFF) | (120u << 24), u(8.0f));
        draw->AddRect(pos, corner, hovered ? menu::kGold : menu::kPanelBorder, u(8.0f));
        menu::navCursor(pos, corner, u(8.0f));
        menu::textCentered(draw, fonts.text, pos.x + defaultsW * 0.5f, pos.y + (corner.y - y - fonts.text->FontSize) * 0.5f,
            enabled ? menu::kText : menu::kTextDim, tr(UiText::DefaultsShort));
        return corner.y;
    }

    // The message line (an error in red, a note in grey) and the debug mode note
    float message(float x, float y, float width)
    {
        if (view.message.empty() && !view.debugMode)
            return y;
        float noteW = 0.0f;
        ImGui::PushFont(fonts.smallText);
        if (view.debugMode)
        {
            noteW = ImGui::CalcTextSize(tr(UiText::DebugMode)).x;
            ImGui::SetCursorScreenPos(ImVec2(x + width - noteW, y));
            ImGui::TextColored(menu::color(menu::kError), "%s", tr(UiText::DebugMode));
        }
        if (!view.message.empty())
        {
            ImGui::SetCursorScreenPos(ImVec2(x, y));
            menu::fittedText(view.messageColor, view.message, width - noteW - (noteW > 0.0f ? u(16.0f) : 0.0f));
        }
        ImGui::PopFont();
        return y + fonts.smallText->FontSize;
    }

    // A small rounded button at its text's width; returns its width
    float smallButton(const char* id, const char* label, ImVec2 pos, float height, bool highlighted, bool& pressed)
    {
        const float width = ImGui::CalcTextSize(label).x + u(24.0f);
        bool hovered = false;
        if (area(id, pos, ImVec2(width, height), true, hovered))
            pressed = true;
        const ImVec2 corner(pos.x + width, pos.y + height);
        const ImU32 fill = highlighted ? (hovered ? menu::kGoldHoverBottom : menu::kGoldBottom) : (hovered ? menu::kButtonHover : menu::kButtonDark);
        draw->AddRectFilled(pos, corner, fill, u(8.0f));
        draw->AddRect(pos, corner, highlighted ? menu::kGold : (hovered ? menu::kGold : menu::kPanelBorder), u(8.0f));
        menu::navCursor(pos, corner, u(8.0f));
        menu::textCentered(draw, fonts.text, pos.x + width * 0.5f, pos.y + (height - fonts.text->FontSize) * 0.5f,
            highlighted ? menu::kGoldText : menu::kText, label);
        return width;
    }

    // Exit, About, Update (only with a newer version) and Browse from rightX
    // to the left; returns where they start
    float barButtons(float rightX, float y, float height)
    {
        const float gap = u(8.0f);
        float x = rightX;
        struct Button { const char* id; UiText text; bool* pressed; bool show; bool highlighted; };
        const Button buttons[] = {
            { "##exit", UiText::ExitShort, &intent.exit, true, false },
            { "##about", UiText::About, &intent.about, true, false },
            { "##update", UiText::Update, &intent.update, view.canUpdate && view.updateAvailable, true },
            { "##browse", view.importGame ? UiText::ImportGame : UiText::Browse, &intent.browse, view.canBrowse, false },
        };
        for (const Button& button : buttons)
        {
            if (!button.show)
                continue;
            const char* label = tr(button.text);
            const float width = ImGui::CalcTextSize(label).x + u(24.0f);
            x -= width;
            smallButton(button.id, label, ImVec2(x, y), height, button.highlighted, *button.pressed);
            x -= gap;
        }
        if (!view.version.empty())
        {
            const std::string version = "v" + view.version;
            ImGui::PushFont(fonts.smallText);
            const float width = ImGui::CalcTextSize(version.c_str()).x;
            x -= width + u(4.0f);
            ImGui::SetCursorScreenPos(ImVec2(x, y + (height - fonts.smallText->FontSize) * 0.5f));
            ImGui::TextColored(menu::color(menu::kTextDim), "%s", version.c_str());
            ImGui::PopFont();
            x -= gap;
        }
        return x;
    }

    // The data folder: a dot (green: ready, red: missing or not valid), its
    // label and its path, up to maxX
    void dataFolder(float x, float y, float height, float maxX)
    {
        const float dot = u(8.0f);
        draw->AddCircleFilled(ImVec2(x + dot * 0.5f, y + height * 0.5f), dot * 0.5f, view.dataFolderValid ? menu::kOk : menu::kError);
        x += dot + u(10.0f);
        ImGui::PushFont(fonts.smallText);
        const char* label = tr(UiText::GameData);
        ImGui::SetCursorScreenPos(ImVec2(x, y + (height - fonts.smallText->FontSize) * 0.5f));
        ImGui::TextColored(menu::color(menu::kTextDim), "%s", label);
        x += ImGui::CalcTextSize(label).x + u(14.0f);
        ImGui::PopFont();
        ImGui::SetCursorScreenPos(ImVec2(x, y + (height - fonts.text->FontSize) * 0.5f));
        menu::fittedText(view.dataFolderValid ? menu::kText : menu::kError,
            view.dataFolder.empty() ? std::string(tr(UiText::DataFolderMissing)) : view.dataFolder, (std::max)(0.0f, maxX - x));
    }

    // The bar at the bottom: data folder on the left, buttons on the right
    // (stacked: the buttons under the folder); returns the y after it
    float bottomBar(float x, float y, float width, bool stacked)
    {
        const float padding = u(12.0f);
        const float buttonH = m.pressHeight(30.0f);
        const float lineH = buttonH;
        const float barH = stacked ? padding * 3.0f + lineH + buttonH : padding * 2.0f + buttonH;
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + barH), (menu::kPanel & 0x00FFFFFF) | (230u << 24), u(10.0f));
        draw->AddRect(ImVec2(x, y), ImVec2(x + width, y + barH), menu::kPanelBorder, u(10.0f));
        if (stacked)
        {
            dataFolder(x + padding, y + padding, lineH, x + width - padding);
            barButtons(x + width - padding, y + padding * 2.0f + lineH, buttonH);
        }
        else
        {
            const float buttonsStart = barButtons(x + width - padding, y + padding, buttonH);
            dataFolder(x + padding + u(4.0f), y + padding, buttonH, buttonsStart - u(12.0f));
        }
        return y + barH;
    }
};

inline void background(ImDrawList* draw, const LauncherView& view, ImVec2 min, ImVec2 max)
{
    draw->AddRectFilled(min, max, menu::kBackground);
    if (view.cover && view.cover->background)
        fillImage(draw, view.cover->background, view.cover->width, view.cover->height, min, max, 0.0f);
}

inline void landscape(Parts& p, ImVec2 origin, ImVec2 area)
{
    const menu::Metrics& m = p.m;
    const float margin = m.px(40.0f);
    const float blockW = m.px(kDesignLandscape.x);
    const float left = origin.x + (std::max)(0.0f, (area.x - blockW) * 0.5f);
    const float width = (std::min)(area.x, blockW);

    p.header(left + margin, left + width - margin, origin.y + m.px(22.0f));

    // Cover on the left, the rest on its right, centred in the height left
    const float coverW = m.px(300.0f), coverH = m.px(400.0f);
    const float barH = m.px(24.0f) + m.pressHeight(30.0f);
    const float top = origin.y + m.px(70.0f);
    const float free = area.y - (top - origin.y) - barH - m.px(24.0f);
    const float middleTop = top + (std::max)(0.0f, (free - coverH) * 0.5f);
    p.cover(ImVec2(left + margin, middleTop), ImVec2(left + margin + coverW, middleTop + coverH));

    const float x = left + margin + coverW + m.px(40.0f);
    const float columnW = left + width - margin - x;
    float y = p.title(x, middleTop + m.px(2.0f), columnW, false);
    y = (std::max)(y + m.px(12.0f), middleTop + m.px(120.0f));
    p.playButton(ImVec2(x, y), ImVec2(columnW, m.pressHeight(54.0f)));
    y += m.pressHeight(54.0f) + m.px(22.0f);
    y = p.cards(x, y, columnW, false);
    y = p.settingsRow(x, y + m.px(20.0f), columnW);
    y = p.message(x, y + m.px(14.0f), columnW);

    const float barTop = (std::max)((std::max)(y, middleTop + coverH) + m.px(16.0f), origin.y + area.y - barH - m.px(20.0f));
    const float end = p.bottomBar(left + margin, barTop, width - margin * 2.0f, false);
    ImGui::SetCursorScreenPos(ImVec2(origin.x, end + m.px(16.0f)));
    ImGui::Dummy(ImVec2(1.0f, 1.0f));
}

inline void portrait(Parts& p, ImVec2 origin, ImVec2 area)
{
    const menu::Metrics& m = p.m;
    const float colW = (std::min)(area.x - m.px(32.0f), m.px(440.0f));
    const float x = origin.x + (area.x - colW) * 0.5f;
    float y = origin.y + m.px(18.0f);

    p.header(x, x + colW, y);
    y += m.px(48.0f);
    y = p.title(x, y, colW, true) + m.px(14.0f);

    const float coverW = (std::min)(colW * 0.62f, m.px(250.0f));
    const float coverH = coverW * 4.0f / 3.0f;
    const float coverX = x + (colW - coverW) * 0.5f;
    p.cover(ImVec2(coverX, y), ImVec2(coverX + coverW, y + coverH));
    y += coverH + m.px(24.0f);

    p.playButton(ImVec2(x, y), ImVec2(colW, m.pressHeight(54.0f)));
    y += m.pressHeight(54.0f) + m.px(20.0f);
    y = p.cards(x, y, colW, true);
    y = p.settingsRow(x, y + m.px(16.0f), colW);
    y = p.message(x, y + m.px(12.0f), colW);
    y = p.bottomBar(x, y + m.px(18.0f), colW, true);
    ImGui::SetCursorScreenPos(ImVec2(origin.x, y + m.px(16.0f)));
    ImGui::Dummy(ImVec2(1.0f, 1.0f));
}

} // namespace timeGateScreen

// Draws the whole screen in a window covering the display and returns what
// the user did; the host opens its own windows afterwards
inline LauncherIntent drawTimeGateLauncherScreen(const LauncherView& view, const menu::Metrics& metrics, const menu::Fonts& fonts)
{
    LauncherIntent intent;
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##launcher", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
    timeGateScreen::background(ImGui::GetWindowDrawList(), view, ImVec2(0, 0), io.DisplaySize);

    ImGui::BeginChild("##launcherContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NavFlattened);
    ImGui::PushFont(fonts.text);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 area = ImGui::GetContentRegionAvail();
    timeGateScreen::Parts parts{ view, metrics, fonts, intent, ImGui::GetWindowDrawList(), view.dataFolderValid };
    if (metrics.portrait())
        timeGateScreen::portrait(parts, origin, area);
    else
        timeGateScreen::landscape(parts, origin, area);
    menu::dragScroll();
    ImGui::PopFont();
    ImGui::EndChild();
    ImGui::End();
    return intent;
}

#endif
