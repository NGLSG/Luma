#pragma once


#include <stdint.h>

namespace ImGui
{
    struct Link;
    struct MarkdownConfig;

    struct MarkdownLinkCallbackData
    {
        const char* text;
        int textLength;
        const char* link;
        int linkLength;
        void* userData;
        bool isImage;
    };

    struct MarkdownTooltipCallbackData
    {
        MarkdownLinkCallbackData linkData;
        const char* linkIcon;
    };

    struct MarkdownImageData
    {
        bool isValid = false;
        bool useLinkCallback = false;
        ImTextureID user_texture_id = 0;
        ImVec2 size = ImVec2(100.0f, 100.0f);
        ImVec2 uv0 = ImVec2(0, 0);
        ImVec2 uv1 = ImVec2(1, 1);
        ImVec4 tint_col = ImVec4(1, 1, 1, 1);
        ImVec4 border_col = ImVec4(0, 0, 0, 0);
        ImVec4 bg_col = ImVec4(0, 0, 0, 0);
    };

    enum class MarkdownFormatType
    {
        NORMAL_TEXT,
        HEADING,
        UNORDERED_LIST,
        LINK,
        EMPHASIS,
    };

    struct MarkdownFormatInfo
    {
        MarkdownFormatType type = MarkdownFormatType::NORMAL_TEXT;
        int32_t level = 0;
        bool itemHovered = false;
        const MarkdownConfig* config = NULL;
        const char* text = NULL;
        int32_t textLength = 0;
    };

    typedef void MarkdownLinkCallback(MarkdownLinkCallbackData data);
    typedef void MarkdownTooltipCallback(MarkdownTooltipCallbackData data);

    inline void defaultMarkdownTooltipCallback(MarkdownTooltipCallbackData data_)
    {
        if (data_.linkData.isImage)
        {
            ImGui::SetTooltip("%.*s", data_.linkData.linkLength, data_.linkData.link);
        }
        else
        {
            ImGui::SetTooltip("%s Open in browser\n%.*s", data_.linkIcon, data_.linkData.linkLength,
                              data_.linkData.link);
        }
    }

    typedef MarkdownImageData MarkdownImageCallback(MarkdownLinkCallbackData data);
    typedef void MarkdownFormalCallback(const MarkdownFormatInfo& markdownFormatInfo_, bool start_);

    inline void defaultMarkdownFormatCallback(const MarkdownFormatInfo& markdownFormatInfo_, bool start_);

    struct MarkdownHeadingFormat
    {
        ImFont* font;
        bool separator;
#ifdef IMGUI_HAS_TEXTURES
        float fontSize = 0.0f;
#endif
    };


    struct MarkdownConfig
    {
        static constexpr int NUMHEADINGS = 3;

        MarkdownLinkCallback* linkCallback = NULL;
        MarkdownTooltipCallback* tooltipCallback = NULL;
        MarkdownImageCallback* imageCallback = NULL;
        const char* linkIcon = "";
        MarkdownHeadingFormat headingFormats[NUMHEADINGS] = {{NULL, true}, {NULL, true}, {NULL, true}};
        void* userData = NULL;
        MarkdownFormalCallback* formatCallback = defaultMarkdownFormatCallback;
    };


    inline void Markdown(const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_);


    struct TextRegion;
    struct Line;
    inline void UnderLine(ImColor col_);
    inline void RenderLine(const char* markdown_, Line& line_, TextRegion& textRegion_,
                           const MarkdownConfig& mdConfig_);

    struct TextRegion
    {
        TextRegion() : indentX(0.0f)
        {
        }

        ~TextRegion()
        {
            ResetIndent();
        }


        void RenderTextWrapped(const char* text_, const char* text_end_, bool bIndentToHere_ = false)
        {
#if IMGUI_VERSION_NUM >= 19197
            float fontSize = ImGui::GetFontSize();
#else
            float scale = ImGui::GetIO().FontGlobalScale;
#endif
            float widthLeft = GetContentRegionAvail().x;
#if IMGUI_VERSION_NUM >= 19197
            const char* endLine = ImGui::GetFont()->CalcWordWrapPosition(fontSize, text_, text_end_, widthLeft);
#else
            const char* endLine = ImGui::GetFont()->CalcWordWrapPositionA(scale, text_, text_end_, widthLeft);
#endif
            ImGui::TextUnformatted(text_, endLine);
            if (bIndentToHere_)
            {
                float indentNeeded = GetContentRegionAvail().x - widthLeft;
                if (indentNeeded)
                {
                    ImGui::Indent(indentNeeded);
                    indentX += indentNeeded;
                }
            }
            widthLeft = GetContentRegionAvail().x;
            while (endLine < text_end_)
            {
                text_ = endLine;
                if (*text_ == ' ') { ++text_; }
#if IMGUI_VERSION_NUM >= 19197
                endLine = ImGui::GetFont()->CalcWordWrapPosition(fontSize, text_, text_end_, widthLeft);
#else
                endLine = ImGui::GetFont()->CalcWordWrapPositionA(scale, text_, text_end_, widthLeft);
#endif
                if (text_ == endLine)
                {
                    endLine++;
                }
                ImGui::TextUnformatted(text_, endLine);
            }
        }

        void RenderListTextWrapped(const char* text_, const char* text_end_)
        {
            ImGui::Bullet();
            ImGui::SameLine();
            RenderTextWrapped(text_, text_end_, true);
        }

        bool RenderLinkText(const char* text_, const char* text_end_, const Link& link_,
                            const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_);

        void RenderLinkTextWrapped(const char* text_, const char* text_end_, const Link& link_,
                                   const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_,
                                   bool bIndentToHere_ = false);

        void ResetIndent()
        {
            if (indentX > 0.0f)
            {
                ImGui::Unindent(indentX);
            }
            indentX = 0.0f;
        }

    private:
        float indentX;
    };


    struct Line
    {
        bool isHeading = false;
        bool isEmphasis = false;
        bool isUnorderedListStart = false;
        bool isLeadingSpace = true;
        int leadSpaceCount = 0;
        int headingCount = 0;
        int emphasisCount = 0;
        int lineStart = 0;
        int lineEnd = 0;
        int lastRenderPosition = 0;
    };

    struct TextBlock
    {
        int start = 0;
        int stop = 0;

        int size() const
        {
            return stop - start;
        }
    };

    struct Link
    {
        enum LinkState
        {
            NO_LINK,
            HAS_SQUARE_BRACKET_OPEN,
            HAS_SQUARE_BRACKETS,
            HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN,
        };

        LinkState state = NO_LINK;
        TextBlock text;
        TextBlock url;
        bool isImage = false;
        int num_brackets_open = 0;
    };

    struct Emphasis
    {
        enum EmphasisState
        {
            NONE,
            LEFT,
            MIDDLE,
            RIGHT,
        };

        EmphasisState state = NONE;
        TextBlock text;
        char sym;
    };

    inline void UnderLine(ImColor col_)
    {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        min.y = max.y;
        ImGui::GetWindowDrawList()->AddLine(min, max, col_, 1.0f);
    }

    inline void RenderLine(const char* markdown_, Line& line_, TextRegion& textRegion_, const MarkdownConfig& mdConfig_)
    {
        int indentStart = 0;
        if (line_.isUnorderedListStart)
        {
            indentStart = 1;
        }
        for (int j = indentStart; j < line_.leadSpaceCount / 2; ++j)
        {
            ImGui::Indent();
        }


        MarkdownFormatInfo formatInfo;
        formatInfo.config = &mdConfig_;
        int textStart = line_.lastRenderPosition + 1;
        int textSize = line_.lineEnd - textStart;
        if (line_.isUnorderedListStart)
        {
            formatInfo.type = MarkdownFormatType::UNORDERED_LIST;
            mdConfig_.formatCallback(formatInfo, true);
            const char* text = markdown_ + textStart + 1;
            textRegion_.RenderListTextWrapped(text, text + textSize - 1);
        }
        else if (line_.isHeading)
        {
            formatInfo.level = line_.headingCount;
            formatInfo.type = MarkdownFormatType::HEADING;
            const char* text = markdown_ + textStart + 1;
            formatInfo.text = text;
            formatInfo.textLength = textSize - 1;
            mdConfig_.formatCallback(formatInfo, true);
            textRegion_.RenderTextWrapped(text, text + textSize - 1);
        }
        else if (line_.isEmphasis)
        {
            formatInfo.level = line_.emphasisCount;
            formatInfo.type = MarkdownFormatType::EMPHASIS;
            mdConfig_.formatCallback(formatInfo, true);
            const char* text = markdown_ + textStart;
            textRegion_.RenderTextWrapped(text, text + textSize);
        }
        else
        {
            formatInfo.type = MarkdownFormatType::NORMAL_TEXT;
            mdConfig_.formatCallback(formatInfo, true);
            const char* text = markdown_ + textStart;
            textRegion_.RenderTextWrapped(text, text + textSize);
        }
        mdConfig_.formatCallback(formatInfo, false);


        for (int j = indentStart; j < line_.leadSpaceCount / 2; ++j)
        {
            ImGui::Unindent();
        }
    }


    inline void Markdown(const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_)
    {
        static const char* linkHoverStart = NULL;
        ImGuiStyle& style = ImGui::GetStyle();
        Line line;
        Link link;
        Emphasis em;
        TextRegion textRegion;

        char c = 0;
        for (int i = 0; i < (int)markdownLength_; ++i)
        {
            c = markdown_[i];
            if (c == 0) { break; }


            if (line.isLeadingSpace)
            {
                if (c == ' ')
                {
                    ++line.leadSpaceCount;
                    continue;
                }
                else
                {
                    line.isLeadingSpace = false;
                    line.lastRenderPosition = i - 1;
                    if ((c == '*') && (line.leadSpaceCount >= 2))
                    {
                        if (((int)markdownLength_ > i + 1) && (markdown_[i + 1] == ' '))
                        {
                            line.isUnorderedListStart = true;
                            ++i;
                            ++line.lastRenderPosition;
                        }
                    }
                    else if (c == '#')
                    {
                        line.headingCount++;
                        bool bContinueChecking = true;
                        int j = i;
                        while (++j < (int)markdownLength_ && bContinueChecking)
                        {
                            c = markdown_[j];
                            switch (c)
                            {
                            case '#':
                                line.headingCount++;
                                break;
                            case ' ':
                                line.lastRenderPosition = j - 1;
                                i = j;
                                line.isHeading = true;
                                bContinueChecking = false;
                                break;
                            default:
                                line.isHeading = false;
                                bContinueChecking = false;
                                break;
                            }
                        }
                        if (line.isHeading)
                        {
                            em = Emphasis();
                            continue;
                        }
                    }
                }
            }


            switch (link.state)
            {
            case Link::NO_LINK:
                if (c == '[' && !line.isHeading)
                {
                    link.state = Link::HAS_SQUARE_BRACKET_OPEN;
                    link.text.start = i + 1;
                    if (i > 0 && markdown_[i - 1] == '!')
                    {
                        link.isImage = true;
                    }
                }
                break;
            case Link::HAS_SQUARE_BRACKET_OPEN:
                if (c == ']')
                {
                    link.state = Link::HAS_SQUARE_BRACKETS;
                    link.text.stop = i;
                }
                break;
            case Link::HAS_SQUARE_BRACKETS:
                if (c == '(')
                {
                    link.state = Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN;
                    link.url.start = i + 1;
                    link.num_brackets_open = 1;
                }
                break;
            case Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN:
                if (c == '(')
                {
                    ++link.num_brackets_open;
                }
                else if (c == ')')
                {
                    --link.num_brackets_open;
                }
                if (link.num_brackets_open == 0)
                {
                    em = Emphasis();

                    line.lineEnd = link.text.start - (link.isImage ? 2 : 1);
                    RenderLine(markdown_, line, textRegion, mdConfig_);
                    line.leadSpaceCount = 0;
                    link.url.stop = i;
                    line.isUnorderedListStart = false;
                    ImGui::SameLine(0.0f, 0.0f);
                    if (link.isImage)
                    {
                        bool drawnImage = false;
                        bool useLinkCallback = false;
                        if (mdConfig_.imageCallback)
                        {
                            MarkdownImageData imageData = mdConfig_.imageCallback({
                                markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start,
                                link.url.size(), mdConfig_.userData, true
                            });
                            useLinkCallback = imageData.useLinkCallback;
                            if (imageData.isValid)
                            {
#if IMGUI_VERSION_NUM < 19185
                                if (imageData.bg_col.w > 0.0f)
                                {
                                    ImVec2 p = ImGui::GetCursorScreenPos();
                                    ImGui::GetWindowDrawList()->AddRectFilled(
                                        p, ImVec2(p.x + imageData.size.x, p.y + imageData.size.y),
                                        ImGui::GetColorU32(imageData.bg_col));
                                }
                                ImGui::Image(imageData.user_texture_id, imageData.size, imageData.uv0, imageData.uv1,
                                             imageData.tint_col, imageData.border_col);
#else
                                ImGui::PushStyleColor(ImGuiCol_Border, imageData.border_col);
                                ImGui::ImageWithBg(imageData.user_texture_id, imageData.size, imageData.uv0,
                                                   imageData.uv1, imageData.bg_col, imageData.tint_col);
                                ImGui::PopStyleColor();
#endif
                                drawnImage = true;
                            }
                        }
                        if (!drawnImage)
                        {
                            ImGui::Text("( Image %.*s not loaded )", link.url.size(), markdown_ + link.url.start);
                        }
                        if (ImGui::IsItemHovered())
                        {
                            if (ImGui::IsMouseReleased(0) && mdConfig_.linkCallback && useLinkCallback)
                            {
                                mdConfig_.linkCallback({
                                    markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start,
                                    link.url.size(), mdConfig_.userData, true
                                });
                            }
                            if (link.text.size() > 0 && mdConfig_.tooltipCallback)
                            {
                                mdConfig_.tooltipCallback({
                                    {
                                        markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start,
                                        link.url.size(), mdConfig_.userData, true
                                    },
                                    mdConfig_.linkIcon
                                });
                            }
                        }
                    }
                    else
                    {
                        textRegion.RenderLinkTextWrapped(markdown_ + link.text.start,
                                                         markdown_ + link.text.start + link.text.size(), link,
                                                         markdown_, mdConfig_, &linkHoverStart, false);
                    }
                    ImGui::SameLine(0.0f, 0.0f);

                    link = Link();
                    line.lastRenderPosition = i;
                    break;
                }
            }


            switch (em.state)
            {
            case Emphasis::NONE:
                if (link.state == Link::NO_LINK && !line.isHeading)
                {
                    int next = i + 1;
                    int prev = i - 1;
                    if ((c == '*' || c == '_')
                        && (i == line.lineStart
                            || markdown_[prev] == ' '
                            || markdown_[prev] == '\t')
                        && (int)markdownLength_ > next
                        && markdown_[next] != ' '
                        && markdown_[next] != '\n'
                        && markdown_[next] != '\t')
                    {
                        em.state = Emphasis::LEFT;
                        em.sym = c;
                        em.text.start = i;
                        line.emphasisCount = 1;
                        continue;
                    }
                }
                break;
            case Emphasis::LEFT:
                if (em.sym == c)
                {
                    ++line.emphasisCount;
                    continue;
                }
                else
                {
                    em.text.start = i;
                    em.state = Emphasis::MIDDLE;
                }
                break;
            case Emphasis::MIDDLE:
                if (em.sym == c)
                {
                    em.state = Emphasis::RIGHT;
                    em.text.stop = i;
                }
                else
                {
                    break;
                }
#if __cplusplus >= 201703L
                [[fallthrough]];
#endif
            case Emphasis::RIGHT:
                if (em.sym == c)
                {
                    if (line.emphasisCount < 3 && (i - em.text.stop + 1 == line.emphasisCount))
                    {
                        int lineEnd = em.text.start - line.emphasisCount;
                        if (lineEnd > line.lineStart)
                        {
                            line.lineEnd = lineEnd;
                            RenderLine(markdown_, line, textRegion, mdConfig_);
                            ImGui::SameLine(0.0f, 0.0f);
                            line.isUnorderedListStart = false;
                            line.leadSpaceCount = 0;
                        }
                        line.isEmphasis = true;
                        line.lastRenderPosition = em.text.start - 1;
                        line.lineStart = em.text.start;
                        line.lineEnd = em.text.stop;
                        RenderLine(markdown_, line, textRegion, mdConfig_);
                        ImGui::SameLine(0.0f, 0.0f);
                        line.isEmphasis = false;
                        line.lastRenderPosition = i;
                        em = Emphasis();
                    }
                    continue;
                }
                else
                {
                    em.state = Emphasis::NONE;

                    int start = em.text.start - line.emphasisCount;
                    if (start < line.lineStart)
                    {
                        line.lineEnd = line.lineStart;
                        line.lineStart = start;
                        line.lastRenderPosition = start - 1;
                        RenderLine(markdown_, line, textRegion, mdConfig_);
                        line.lineStart = line.lineEnd;
                        line.lastRenderPosition = line.lineStart - 1;
                    }
                }
                break;
            }


            if (c == '\n')
            {
                line.lineEnd = i;
                if (em.state == Emphasis::MIDDLE && line.emphasisCount >= 3 &&
                    (line.lineStart + line.emphasisCount) == i)
                {
                    ImGui::Separator();
                }
                else
                {
                    RenderLine(markdown_, line, textRegion, mdConfig_);
                }


                line = Line();
                em = Emphasis();

                line.lineStart = i + 1;
                line.lastRenderPosition = i;

                textRegion.ResetIndent();


                link = Link();
            }
        }

        if (em.state == Emphasis::LEFT && line.emphasisCount >= 3)
        {
            ImGui::Separator();
        }
        else
        {
            if (markdownLength_ && line.lineStart < (int)markdownLength_ && markdown_[line.lineStart] != 0)
            {
                line.lineEnd = (int)markdownLength_;
                if (0 == markdown_[line.lineEnd - 1])
                {
                    --line.lineEnd;
                }
                RenderLine(markdown_, line, textRegion, mdConfig_);
            }
        }
    }

    inline bool TextRegion::RenderLinkText(const char* text_, const char* text_end_, const Link& link_,
                                           const char* markdown_, const MarkdownConfig& mdConfig_,
                                           const char** linkHoverStart_)
    {
        MarkdownFormatInfo formatInfo;
        formatInfo.config = &mdConfig_;
        formatInfo.type = MarkdownFormatType::LINK;
        mdConfig_.formatCallback(formatInfo, true);
        ImGui::PushTextWrapPos(-1.0f);
        ImGui::TextUnformatted(text_, text_end_);
        ImGui::PopTextWrapPos();

        bool bThisItemHovered = ImGui::IsItemHovered();
        if (bThisItemHovered)
        {
            *linkHoverStart_ = markdown_ + link_.text.start;
        }
        bool bHovered = bThisItemHovered || (*linkHoverStart_ == (markdown_ + link_.text.start));

        formatInfo.itemHovered = bHovered;
        mdConfig_.formatCallback(formatInfo, false);

        if (bHovered)
        {
            if (ImGui::IsMouseReleased(0) && mdConfig_.linkCallback)
            {
                mdConfig_.linkCallback({
                    markdown_ + link_.text.start, link_.text.size(), markdown_ + link_.url.start, link_.url.size(),
                    mdConfig_.userData, false
                });
            }
            if (mdConfig_.tooltipCallback)
            {
                mdConfig_.tooltipCallback({
                    {
                        markdown_ + link_.text.start, link_.text.size(), markdown_ + link_.url.start, link_.url.size(),
                        mdConfig_.userData, false
                    },
                    mdConfig_.linkIcon
                });
            }
        }
        return bThisItemHovered;
    }


    inline bool IsCharInsideWord(char c_)
    {
        return c_ != ' ' && c_ != '.' && c_ != ',' && c_ != ';' && c_ != '!' && c_ != '?' && c_ != '\"';
    }

    inline void TextRegion::RenderLinkTextWrapped(const char* text_, const char* text_end_, const Link& link_,
                                                  const char* markdown_, const MarkdownConfig& mdConfig_,
                                                  const char** linkHoverStart_, bool bIndentToHere_)
    {
#if IMGUI_VERSION_NUM >= 19197
        float fontSize = ImGui::GetFontSize();
#else
        float scale = ImGui::GetIO().FontGlobalScale;
#endif
        float widthLeft = GetContentRegionAvail().x;
        const char* endLine = text_;
        if (widthLeft > 0.0f)
        {
#if IMGUI_VERSION_NUM >= 19197
            endLine = ImGui::GetFont()->CalcWordWrapPosition(fontSize, text_, text_end_, widthLeft);
#else
            endLine = ImGui::GetFont()->CalcWordWrapPositionA(scale, text_, text_end_, widthLeft);
#endif
        }

        if (endLine > text_ && endLine < text_end_)
        {
            if (IsCharInsideWord(*endLine))
            {
                float widthNextLine = widthLeft + GetCursorScreenPos().x - GetWindowPos().x;
#if IMGUI_VERSION_NUM >= 19197
                const char* endNextLine = ImGui::GetFont()->CalcWordWrapPosition(
                    fontSize, text_, text_end_, widthNextLine);
#else
                const char* endNextLine = ImGui::GetFont()->CalcWordWrapPositionA(scale, text_, text_end_, widthLeft);
#endif
                if (endNextLine == text_end_ || (endNextLine <= text_end_ && !IsCharInsideWord(*endNextLine)))
                {
                    endLine = text_;
                }
            }
        }
        bool bHovered = RenderLinkText(text_, endLine, link_, markdown_, mdConfig_, linkHoverStart_);
        if (bIndentToHere_)
        {
            float indentNeeded = GetContentRegionAvail().x - widthLeft;
            if (indentNeeded)
            {
                ImGui::Indent(indentNeeded);
                indentX += indentNeeded;
            }
        }
        widthLeft = GetContentRegionAvail().x;
        while (endLine < text_end_)
        {
            text_ = endLine;
            if (*text_ == ' ') { ++text_; }
#if IMGUI_VERSION_NUM >= 19197
            endLine = ImGui::GetFont()->CalcWordWrapPosition(fontSize, text_, text_end_, widthLeft);
#else
            endLine = ImGui::GetFont()->CalcWordWrapPositionA(scale, text_, text_end_, widthLeft);
#endif
            if (text_ == endLine)
            {
                endLine++;
            }
            bool bThisLineHovered = RenderLinkText(text_, endLine, link_, markdown_, mdConfig_, linkHoverStart_);
            bHovered = bHovered || bThisLineHovered;
        }
        if (!bHovered && *linkHoverStart_ == markdown_ + link_.text.start)
        {
            *linkHoverStart_ = NULL;
        }
    }


    inline void defaultMarkdownFormatCallback(const MarkdownFormatInfo& markdownFormatInfo_, bool start_)
    {
        switch (markdownFormatInfo_.type)
        {
        case MarkdownFormatType::NORMAL_TEXT:
            break;
        case MarkdownFormatType::EMPHASIS:
            {
                MarkdownHeadingFormat fmt;


                if (markdownFormatInfo_.level == 1)
                {
                    if (start_)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    }
                    else
                    {
                        ImGui::PopStyleColor();
                    }
                }
                else
                {
                    fmt = markdownFormatInfo_.config->headingFormats[MarkdownConfig::NUMHEADINGS - 1];
                    if (start_)
                    {
                        if (fmt.font)
                        {
#ifdef IMGUI_HAS_TEXTURES
                            ImGui::PushFont(fmt.font, 0.0f);
#else
                            ImGui::PushFont(fmt.font);
#endif
                        }
                    }
                    else
                    {
                        if (fmt.font)
                        {
                            ImGui::PopFont();
                        }
                    }
                }
                break;
            }
        case MarkdownFormatType::HEADING:
            {
                MarkdownHeadingFormat fmt;
                if (markdownFormatInfo_.level > MarkdownConfig::NUMHEADINGS)
                {
                    fmt = markdownFormatInfo_.config->headingFormats[MarkdownConfig::NUMHEADINGS - 1];
                }
                else
                {
                    fmt = markdownFormatInfo_.config->headingFormats[markdownFormatInfo_.level - 1];
                }
                if (start_)
                {
                    if (fmt.font)
                    {
#ifdef IMGUI_HAS_TEXTURES
                        ImGui::PushFont(fmt.font, fmt.fontSize > 0.0f ? fmt.fontSize : fmt.font->LegacySize);
#else
                        ImGui::PushFont(fmt.font);
#endif
                    }
                    ImGui::NewLine();
                }
                else
                {
                    if (fmt.separator)
                    {
                        ImGui::Separator();
                        ImGui::NewLine();
                    }
                    else
                    {
                        ImGui::NewLine();
                    }
                    if (fmt.font)
                    {
                        ImGui::PopFont();
                    }
                }
                break;
            }
        case MarkdownFormatType::UNORDERED_LIST:
            break;
        case MarkdownFormatType::LINK:
            if (start_)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
            }
            else
            {
                ImGui::PopStyleColor();
                if (markdownFormatInfo_.itemHovered)
                {
                    ImGui::UnderLine(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
                }
                else
                {
                    ImGui::UnderLine(ImGui::GetStyle().Colors[ImGuiCol_Button]);
                }
            }
            break;
        }
    }
}
