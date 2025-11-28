using System;
using System.Runtime.InteropServices;

namespace Luma.SDK.Plugins;




public static partial class ImGui
{
    private const string DllName = "LumaEngine";

    #region Window

    [LibraryImport(DllName, EntryPoint = "ImGui_Begin")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool Begin([MarshalAs(UnmanagedType.LPStr)] string name);

    [LibraryImport(DllName, EntryPoint = "ImGui_BeginWithOpen")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool Begin([MarshalAs(UnmanagedType.LPStr)] string name, [MarshalAs(UnmanagedType.Bool)] ref bool open);

    [LibraryImport(DllName, EntryPoint = "ImGui_End")]
    public static partial void End();

    #endregion

    #region Text

    [LibraryImport(DllName, EntryPoint = "ImGui_Text")]
    public static partial void Text([MarshalAs(UnmanagedType.LPStr)] string text);

    [LibraryImport(DllName, EntryPoint = "ImGui_TextColored")]
    public static partial void TextColored(float r, float g, float b, float a, [MarshalAs(UnmanagedType.LPStr)] string text);

    [LibraryImport(DllName, EntryPoint = "ImGui_TextDisabled")]
    public static partial void TextDisabled([MarshalAs(UnmanagedType.LPStr)] string text);

    [LibraryImport(DllName, EntryPoint = "ImGui_TextWrapped")]
    public static partial void TextWrapped([MarshalAs(UnmanagedType.LPStr)] string text);

    [LibraryImport(DllName, EntryPoint = "ImGui_LabelText")]
    public static partial void LabelText([MarshalAs(UnmanagedType.LPStr)] string label, [MarshalAs(UnmanagedType.LPStr)] string text);

    #endregion

    #region Buttons

    [LibraryImport(DllName, EntryPoint = "ImGui_Button")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool Button([MarshalAs(UnmanagedType.LPStr)] string label);

    [LibraryImport(DllName, EntryPoint = "ImGui_ButtonEx")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool Button([MarshalAs(UnmanagedType.LPStr)] string label, float width, float height);

    [LibraryImport(DllName, EntryPoint = "ImGui_SmallButton")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool SmallButton([MarshalAs(UnmanagedType.LPStr)] string label);

    [LibraryImport(DllName, EntryPoint = "ImGui_Checkbox")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool Checkbox([MarshalAs(UnmanagedType.LPStr)] string label, [MarshalAs(UnmanagedType.Bool)] ref bool value);

    #endregion

    #region Input

    [LibraryImport(DllName, EntryPoint = "ImGui_InputTextCallback")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool InputTextNative(
        [MarshalAs(UnmanagedType.LPStr)] string label,
        [MarshalAs(UnmanagedType.LPStr)] string text,
        IntPtr outBuffer,
        int outBufferSize,
        [MarshalAs(UnmanagedType.Bool)] ref bool changed);

    
    
    
    
    
    
    
    public static bool InputText(string label, ref string text, int maxLength = 256)
    {
        IntPtr buffer = Marshal.AllocHGlobal(maxLength);
        try
        {
            bool changed = false;
            InputTextNative(label, text ?? "", buffer, maxLength, ref changed);
            if (changed)
            {
                text = Marshal.PtrToStringUTF8(buffer) ?? "";
            }
            return changed;
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }

    [LibraryImport(DllName, EntryPoint = "ImGui_InputTextMultilineCallback")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool InputTextMultilineNative(
        [MarshalAs(UnmanagedType.LPStr)] string label,
        [MarshalAs(UnmanagedType.LPStr)] string text,
        IntPtr outBuffer,
        int outBufferSize,
        float width,
        float height,
        [MarshalAs(UnmanagedType.Bool)] ref bool changed);

    
    
    
    
    
    
    
    
    
    public static bool InputTextMultiline(string label, ref string text, float width = 0, float height = 0, int maxLength = 4096)
    {
        IntPtr buffer = Marshal.AllocHGlobal(maxLength);
        try
        {
            bool changed = false;
            InputTextMultilineNative(label, text ?? "", buffer, maxLength, width, height, ref changed);
            if (changed)
            {
                text = Marshal.PtrToStringUTF8(buffer) ?? "";
            }
            return changed;
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }

    [LibraryImport(DllName, EntryPoint = "ImGui_InputFloat")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool InputFloat([MarshalAs(UnmanagedType.LPStr)] string label, ref float value);

    [LibraryImport(DllName, EntryPoint = "ImGui_InputInt")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool InputInt([MarshalAs(UnmanagedType.LPStr)] string label, ref int value);

    [LibraryImport(DllName, EntryPoint = "ImGui_SliderFloat")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool SliderFloat([MarshalAs(UnmanagedType.LPStr)] string label, ref float value, float min, float max);

    [LibraryImport(DllName, EntryPoint = "ImGui_SliderInt")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool SliderInt([MarshalAs(UnmanagedType.LPStr)] string label, ref int value, int min, int max);

    #endregion

    #region Layout

    [LibraryImport(DllName, EntryPoint = "ImGui_Separator")]
    public static partial void Separator();

    [LibraryImport(DllName, EntryPoint = "ImGui_SameLine")]
    public static partial void SameLine();

    [LibraryImport(DllName, EntryPoint = "ImGui_SameLineEx")]
    public static partial void SameLine(float offsetFromStartX, float spacing);

    [LibraryImport(DllName, EntryPoint = "ImGui_Spacing")]
    public static partial void Spacing();

    [LibraryImport(DllName, EntryPoint = "ImGui_Dummy")]
    public static partial void Dummy(float width, float height);

    [LibraryImport(DllName, EntryPoint = "ImGui_Indent")]
    public static partial void Indent();

    [LibraryImport(DllName, EntryPoint = "ImGui_Unindent")]
    public static partial void Unindent();

    #endregion

    #region Trees & Lists

    [LibraryImport(DllName, EntryPoint = "ImGui_TreeNode")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool TreeNode([MarshalAs(UnmanagedType.LPStr)] string label);

    [LibraryImport(DllName, EntryPoint = "ImGui_TreePop")]
    public static partial void TreePop();

    [LibraryImport(DllName, EntryPoint = "ImGui_CollapsingHeader")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool CollapsingHeader([MarshalAs(UnmanagedType.LPStr)] string label);

    [LibraryImport(DllName, EntryPoint = "ImGui_Selectable")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool Selectable([MarshalAs(UnmanagedType.LPStr)] string label, [MarshalAs(UnmanagedType.Bool)] bool selected);

    #endregion

    #region Menus

    [LibraryImport(DllName, EntryPoint = "ImGui_BeginMenuBar")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool BeginMenuBar();

    [LibraryImport(DllName, EntryPoint = "ImGui_EndMenuBar")]
    public static partial void EndMenuBar();

    [LibraryImport(DllName, EntryPoint = "ImGui_BeginMenu")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool BeginMenu([MarshalAs(UnmanagedType.LPStr)] string label);

    [LibraryImport(DllName, EntryPoint = "ImGui_EndMenu")]
    public static partial void EndMenu();

    [LibraryImport(DllName, EntryPoint = "ImGui_MenuItem")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool MenuItem([MarshalAs(UnmanagedType.LPStr)] string label);

    [LibraryImport(DllName, EntryPoint = "ImGui_MenuItemEx")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool MenuItem([MarshalAs(UnmanagedType.LPStr)] string label, [MarshalAs(UnmanagedType.LPStr)] string shortcut, [MarshalAs(UnmanagedType.Bool)] bool selected);

    #endregion

    #region Popups

    [LibraryImport(DllName, EntryPoint = "ImGui_OpenPopup")]
    public static partial void OpenPopup([MarshalAs(UnmanagedType.LPStr)] string strId);

    [LibraryImport(DllName, EntryPoint = "ImGui_BeginPopup")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool BeginPopup([MarshalAs(UnmanagedType.LPStr)] string strId);

    [LibraryImport(DllName, EntryPoint = "ImGui_BeginPopupModal")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool BeginPopupModal([MarshalAs(UnmanagedType.LPStr)] string name);

    [LibraryImport(DllName, EntryPoint = "ImGui_EndPopup")]
    public static partial void EndPopup();

    [LibraryImport(DllName, EntryPoint = "ImGui_CloseCurrentPopup")]
    public static partial void CloseCurrentPopup();

    #endregion

    #region Utilities

    [LibraryImport(DllName, EntryPoint = "ImGui_IsItemHovered")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool IsItemHovered();

    [LibraryImport(DllName, EntryPoint = "ImGui_IsItemClicked")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool IsItemClicked();

    [LibraryImport(DllName, EntryPoint = "ImGui_SetTooltip")]
    public static partial void SetTooltip([MarshalAs(UnmanagedType.LPStr)] string text);

    [LibraryImport(DllName, EntryPoint = "ImGui_GetWindowWidth")]
    public static partial float GetWindowWidth();

    [LibraryImport(DllName, EntryPoint = "ImGui_GetWindowHeight")]
    public static partial float GetWindowHeight();

    [LibraryImport(DllName, EntryPoint = "ImGui_SetNextWindowSize")]
    public static partial void SetNextWindowSize(float width, float height);

    [LibraryImport(DllName, EntryPoint = "ImGui_SetNextWindowPos")]
    public static partial void SetNextWindowPos(float x, float y);

    [LibraryImport(DllName, EntryPoint = "ImGui_PushID")]
    public static partial void PushID(int id);

    [LibraryImport(DllName, EntryPoint = "ImGui_PushIDStr")]
    public static partial void PushID([MarshalAs(UnmanagedType.LPStr)] string strId);

    [LibraryImport(DllName, EntryPoint = "ImGui_PopID")]
    public static partial void PopID();

    #endregion

    #region Colors

    [LibraryImport(DllName, EntryPoint = "ImGui_ColorEdit3")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool ColorEdit3([MarshalAs(UnmanagedType.LPStr)] string label, ref float r, ref float g, ref float b);

    [LibraryImport(DllName, EntryPoint = "ImGui_ColorEdit4")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool ColorEdit4([MarshalAs(UnmanagedType.LPStr)] string label, ref float r, ref float g, ref float b, ref float a);

    #endregion

    #region Drag

    [LibraryImport(DllName, EntryPoint = "ImGui_DragFloat")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool DragFloat([MarshalAs(UnmanagedType.LPStr)] string label, ref float value, float speed, float min, float max);

    [LibraryImport(DllName, EntryPoint = "ImGui_DragInt")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool DragInt([MarshalAs(UnmanagedType.LPStr)] string label, ref int value, float speed, int min, int max);

    #endregion

    #region Progress

    [LibraryImport(DllName, EntryPoint = "ImGui_ProgressBar")]
    public static partial void ProgressBar(float fraction, float width, float height, [MarshalAs(UnmanagedType.LPStr)] string? overlay);

    
    
    
    public static void ProgressBar(float fraction, string? overlay = null)
    {
        ProgressBar(fraction, -1, 0, overlay);
    }

    #endregion

    #region Child Windows

    [LibraryImport(DllName, EntryPoint = "ImGui_BeginChild")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool BeginChild([MarshalAs(UnmanagedType.LPStr)] string strId, float width, float height, [MarshalAs(UnmanagedType.Bool)] bool border);

    [LibraryImport(DllName, EntryPoint = "ImGui_EndChild")]
    public static partial void EndChild();

    #endregion

    #region Tab Bar

    [LibraryImport(DllName, EntryPoint = "ImGui_BeginTabBar")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool BeginTabBar([MarshalAs(UnmanagedType.LPStr)] string strId);

    [LibraryImport(DllName, EntryPoint = "ImGui_EndTabBar")]
    public static partial void EndTabBar();

    [LibraryImport(DllName, EntryPoint = "ImGui_BeginTabItem")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool BeginTabItem([MarshalAs(UnmanagedType.LPStr)] string label);

    [LibraryImport(DllName, EntryPoint = "ImGui_EndTabItem")]
    public static partial void EndTabItem();

    #endregion

    #region Table

    [LibraryImport(DllName, EntryPoint = "ImGui_BeginTable")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool BeginTable([MarshalAs(UnmanagedType.LPStr)] string strId, int columns);

    [LibraryImport(DllName, EntryPoint = "ImGui_EndTable")]
    public static partial void EndTable();

    [LibraryImport(DllName, EntryPoint = "ImGui_TableNextRow")]
    public static partial void TableNextRow();

    [LibraryImport(DllName, EntryPoint = "ImGui_TableNextColumn")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool TableNextColumn();

    [LibraryImport(DllName, EntryPoint = "ImGui_TableSetupColumn")]
    public static partial void TableSetupColumn([MarshalAs(UnmanagedType.LPStr)] string label);

    [LibraryImport(DllName, EntryPoint = "ImGui_TableHeadersRow")]
    public static partial void TableHeadersRow();

    #endregion

    #region Image

    [LibraryImport(DllName, EntryPoint = "ImGui_Image")]
    public static partial void Image(IntPtr textureId, float width, float height);

    [LibraryImport(DllName, EntryPoint = "ImGui_ImageButton")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static partial bool ImageButton([MarshalAs(UnmanagedType.LPStr)] string strId, IntPtr textureId, float width, float height);

    #endregion
}
