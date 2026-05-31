using Luma.SDK.Plugins;
namespace ColorPalette;

public struct PaletteColor
{
    public float R, G, B, A;

    public PaletteColor(float r, float g, float b, float a = 1.0f)
    {
        R = r; G = g; B = b; A = a;
    }

    public string ToHex()
    {
        int ri = Math.Clamp((int)(R * 255), 0, 255);
        int gi = Math.Clamp((int)(G * 255), 0, 255);
        int bi = Math.Clamp((int)(B * 255), 0, 255);
        return $"#{ri:X2}{gi:X2}{bi:X2}";
    }
}

public class Palette
{
    public string Name { get; set; }
    public List<PaletteColor> Colors { get; set; }

    public Palette(string name)
    {
        Name = name;
        Colors = new List<PaletteColor>();
    }
}

public class ColorPalettePlugin : EditorPlugin
{
    public override string Id => "com.luma.colorpalette";
    public override string Name => "调色板";
    public override string Version => "1.0.0";
    public override string Author => "Luma Engine";
    public override string Description => "调色板管理插件，提供颜色编辑、调色板管理和预设调色板功能。";

    private ColorPalettePanel? _panel;

    public override void OnLoad()
    {
        base.OnLoad();
        _panel = RegisterPanel<ColorPalettePanel>();
        Log.Info($"[{Name}] 插件已加载");
    }

    public override void OnUnload()
    {
        Log.Info($"[{Name}] 插件已卸载");
        base.OnUnload();
    }

    public override void OnMenuBarGUI()
    {
        if (ImGui.BeginMenu("调色板"))
        {
            if (ImGui.MenuItem("打开调色板"))
            {
                if (_panel != null)
                    _panel.IsVisible = true;
            }
            ImGui.EndMenu();
        }
    }

    public override void OnDrawMenuItems(string menuName)
    {
        if (menuName == "窗口")
        {
            if (ImGui.MenuItem("调色板"))
            {
                if (_panel != null)
                    _panel.IsVisible = true;
            }
        }
    }
}

public class ColorPalettePanel : EditorPanel
{
    public override string Title => "调色板";

    private List<Palette> _palettes = new();
    private int _selectedPaletteIndex = 0;
    private float _editR = 1.0f, _editG = 0.5f, _editB = 0.2f, _editA = 1.0f;
    private string _newPaletteName = "";

    public ColorPalettePanel()
    {
        InitPresets();
    }

    private void InitPresets()
    {
        var gb = new Palette("Game Boy");
        gb.Colors.AddRange(new[] {
            new PaletteColor(0.06f, 0.22f, 0.06f),
            new PaletteColor(0.19f, 0.38f, 0.19f),
            new PaletteColor(0.55f, 0.67f, 0.06f),
            new PaletteColor(0.61f, 0.74f, 0.06f),
        });
        _palettes.Add(gb);

        var nes = new Palette("NES");
        nes.Colors.AddRange(new[] {
            new PaletteColor(0.49f, 0.12f, 0.12f),
            new PaletteColor(0.00f, 0.30f, 0.65f),
            new PaletteColor(0.95f, 0.82f, 0.30f),
            new PaletteColor(0.13f, 0.55f, 0.13f),
            new PaletteColor(0.96f, 0.64f, 0.38f),
            new PaletteColor(0.40f, 0.40f, 0.40f),
        });
        _palettes.Add(nes);

        var warm = new Palette("暖色调");
        warm.Colors.AddRange(new[] {
            new PaletteColor(0.80f, 0.20f, 0.10f),
            new PaletteColor(0.93f, 0.49f, 0.13f),
            new PaletteColor(0.98f, 0.75f, 0.18f),
            new PaletteColor(0.85f, 0.35f, 0.20f),
            new PaletteColor(0.70f, 0.15f, 0.15f),
        });
        _palettes.Add(warm);

        var cool = new Palette("冷色调");
        cool.Colors.AddRange(new[] {
            new PaletteColor(0.10f, 0.30f, 0.70f),
            new PaletteColor(0.20f, 0.60f, 0.80f),
            new PaletteColor(0.40f, 0.75f, 0.85f),
            new PaletteColor(0.15f, 0.45f, 0.65f),
            new PaletteColor(0.30f, 0.20f, 0.60f),
        });
        _palettes.Add(cool);

        var bounty = new Palette("像素赏金猎人");
        bounty.Colors.AddRange(new[] {
            new PaletteColor(0.13f, 0.11f, 0.20f),
            new PaletteColor(0.27f, 0.18f, 0.31f),
            new PaletteColor(0.53f, 0.21f, 0.35f),
            new PaletteColor(0.85f, 0.35f, 0.30f),
            new PaletteColor(0.95f, 0.65f, 0.30f),
            new PaletteColor(0.98f, 0.91f, 0.55f),
            new PaletteColor(0.30f, 0.70f, 0.45f),
            new PaletteColor(0.20f, 0.42f, 0.45f),
        });
        _palettes.Add(bounty);
    }

    public override void OnGUI()
    {
        DrawColorEditor();
        ImGui.Separator();
        DrawPaletteManager();
        ImGui.Separator();
        DrawPaletteDisplay();
    }

    private void DrawColorEditor()
    {
        ImGui.Text("颜色编辑器");
        ImGui.ColorEdit4("当前颜色", ref _editR, ref _editG, ref _editB, ref _editA);

        var preview = new PaletteColor(_editR, _editG, _editB, _editA);
        ImGui.SameLine();
        ImGui.Text(preview.ToHex());

        if (ImGui.Button("添加到当前调色板"))
        {
            if (_palettes.Count > 0 && _selectedPaletteIndex < _palettes.Count)
            {
                _palettes[_selectedPaletteIndex].Colors.Add(
                    new PaletteColor(_editR, _editG, _editB, _editA));
                Log.Info($"已添加颜色 {preview.ToHex()} 到 {_palettes[_selectedPaletteIndex].Name}");
            }
        }
    }

    private void DrawPaletteManager()
    {
        ImGui.Text("调色板管理");

        ImGui.InputText("新调色板名称", ref _newPaletteName);
        ImGui.SameLine();
        if (ImGui.Button("新建"))
        {
            if (!string.IsNullOrWhiteSpace(_newPaletteName))
            {
                _palettes.Add(new Palette(_newPaletteName));
                Log.Info($"已创建调色板: {_newPaletteName}");
                _newPaletteName = "";
            }
        }

        if (_palettes.Count > 0 && _selectedPaletteIndex < _palettes.Count)
        {
            ImGui.SameLine();
            if (ImGui.SmallButton("删除当前调色板"))
            {
                Log.Info($"已删除调色板: {_palettes[_selectedPaletteIndex].Name}");
                _palettes.RemoveAt(_selectedPaletteIndex);
                if (_selectedPaletteIndex >= _palettes.Count && _palettes.Count > 0)
                    _selectedPaletteIndex = _palettes.Count - 1;
            }
        }
    }

    private void DrawPaletteDisplay()
    {
        if (_palettes.Count == 0)
        {
            ImGui.Text("暂无调色板");
            return;
        }

        for (int pi = 0; pi < _palettes.Count; pi++)
        {
            var palette = _palettes[pi];
            bool isSelected = pi == _selectedPaletteIndex;
            string header = isSelected ? $"▶ {palette.Name} ({palette.Colors.Count} 色)" : $"  {palette.Name} ({palette.Colors.Count} 色)";

            if (ImGui.CollapsingHeader($"{header}##palette_{pi}"))
            {
                ImGui.PushID(pi);

                if (!isSelected)
                {
                    if (ImGui.SmallButton("选为当前"))
                    {
                        _selectedPaletteIndex = pi;
                    }
                    ImGui.SameLine();
                }

                for (int ci = 0; ci < palette.Colors.Count; ci++)
                {
                    var c = palette.Colors[ci];
                    ImGui.PushID(ci);

                    ImGui.TextColored(c.R, c.G, c.B, c.A, "■");
                    ImGui.SameLine();
                    if (ImGui.Button(c.ToHex()))
                    {
                        Log.Info($"已复制颜色到剪贴板: {c.ToHex()}");
                    }
                    ImGui.SameLine();
                    if (ImGui.SmallButton("×"))
                    {
                        palette.Colors.RemoveAt(ci);
                        Log.Info($"已从 {palette.Name} 移除颜色");
                        ImGui.PopID();
                        break;
                    }

                    if ((ci + 1) % 4 != 0 && ci < palette.Colors.Count - 1)
                        ImGui.SameLine();

                    ImGui.PopID();
                }

                ImGui.PopID();
            }
        }
    }
}
