using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Luma.SourceGenerator
{
    [Generator]
    public class ScriptYamlGenerator : ISourceGenerator
    {
        
        private static readonly SymbolDisplayFormat s_fullyQualifiedFormat =
            new SymbolDisplayFormat(
                globalNamespaceStyle: SymbolDisplayGlobalNamespaceStyle.Omitted,
                typeQualificationStyle: SymbolDisplayTypeQualificationStyle.NameAndContainingTypesAndNamespaces,
                genericsOptions: SymbolDisplayGenericsOptions.IncludeTypeParameters,
                miscellaneousOptions:
                    SymbolDisplayMiscellaneousOptions.EscapeKeywordIdentifiers |
                    SymbolDisplayMiscellaneousOptions.UseSpecialTypes);

        public void Initialize(GeneratorInitializationContext context)
        {
            context.RegisterForSyntaxNotifications(() => new ScriptSyntaxReceiver());
        }

        public void Execute(GeneratorExecutionContext context)
        {
            try
            {
                if (!(context.SyntaxReceiver is ScriptSyntaxReceiver receiver))
                    return;

                var compilation = context.Compilation;
                var yamlBuilder = new StringBuilder();

                yamlBuilder.AppendLine("# Luma引擎脚本元数据配置文件");
                yamlBuilder.AppendLine("# 此文件由Luma.SourceGenerator自动生成，请勿手动修改");
                yamlBuilder.AppendLine();

                GenerateScriptsSection(compilation, receiver.CandidateClasses, yamlBuilder);

                GenerateAvailableTypesSection(compilation, yamlBuilder);

                var yamlContent = yamlBuilder.ToString();

                if (!string.IsNullOrEmpty(yamlContent))
                {
                    context.AddSource("ScriptMetadata.yaml.g.cs", GenerateYamlAsResource(yamlContent));
                }
            }
            catch (Exception ex)
            {
                var descriptor = new DiagnosticDescriptor("SG001", "源代码生成器错误",
                    "源代码生成器执行时发生错误: {0}", "SourceGenerator",
                    DiagnosticSeverity.Warning, true);
                context.ReportDiagnostic(Diagnostic.Create(descriptor, Location.None, ex.Message));
            }
        }

        private string GenerateYamlAsResource(string yamlContent)
        {
            var escapedYaml = yamlContent.Replace("\"", "\"\"");

            return $@"// 此文件由Luma.SourceGenerator自动生成，请勿手动修改
namespace Luma.Generated
{{
    internal static class ScriptMetadata
    {{
        public const string YamlContent = @""{escapedYaml}"";

        public static void WriteToFile(string filePath)
        {{
            System.IO.File.WriteAllText(filePath, YamlContent);
        }}
    }}
}}";
        }

        private void GenerateScriptsSection(Compilation compilation, List<ClassDeclarationSyntax> candidateClasses, StringBuilder yamlBuilder)
        {
            yamlBuilder.AppendLine("Scripts:");

            var hasValidScripts = false;

            foreach (var classDeclaration in candidateClasses)
            {
                var semanticModel = compilation.GetSemanticModel(classDeclaration.SyntaxTree);
                var classSymbol = semanticModel.GetDeclaredSymbol(classDeclaration) as INamedTypeSymbol;

                if (classSymbol == null || !IsScriptClass(classSymbol))
                    continue;

                GenerateClassYaml(yamlBuilder, classSymbol, classDeclaration);
                hasValidScripts = true;
            }

            if (!hasValidScripts)
            {
                yamlBuilder.AppendLine("  # 未发现继承自Script的类");
            }
        }

        private void GenerateAvailableTypesSection(Compilation compilation, StringBuilder yamlBuilder)
        {
            yamlBuilder.AppendLine();
            yamlBuilder.AppendLine("AvailableTypes:");

            var allTypeNames = new HashSet<string>();

            CollectTypesFromNamespace(compilation.GlobalNamespace, allTypeNames);

            foreach (var reference in compilation.References)
            {
                if (compilation.GetAssemblyOrModuleSymbol(reference) is IAssemblySymbol assemblySymbol)
                {
                    CollectTypesFromNamespace(assemblySymbol.GlobalNamespace, allTypeNames);
                }
            }

            if (allTypeNames.Count == 0)
            {
                yamlBuilder.AppendLine("  # 未发现可用类型");
                return;
            }

            var sortedTypeNames = allTypeNames.ToList();
            sortedTypeNames.Sort(StringComparer.Ordinal);

            foreach (var typeName in sortedTypeNames)
            {
                yamlBuilder.AppendLine($"  - \"{typeName}\"");
            }
        }

        private void CollectTypesFromNamespace(INamespaceSymbol namespaceSymbol, HashSet<string> typeNames)
        {
            foreach (var typeSymbol in namespaceSymbol.GetTypeMembers())
            {
                CollectNestedTypes(typeSymbol, typeNames);
            }

            foreach (var childNamespace in namespaceSymbol.GetNamespaceMembers())
            {
                CollectTypesFromNamespace(childNamespace, typeNames);
            }
        }

        private void CollectNestedTypes(INamedTypeSymbol typeSymbol, HashSet<string> typeNames)
        {
            if (typeSymbol.DeclaredAccessibility == Accessibility.Public && !typeSymbol.IsImplicitlyDeclared && !typeSymbol.IsUnboundGenericType)
            {
                
                typeNames.Add(typeSymbol.ToDisplayString(s_fullyQualifiedFormat));
            }

            foreach (var nestedType in typeSymbol.GetTypeMembers())
            {
                CollectNestedTypes(nestedType, typeNames);
            }
        }

        private void GenerateClassYaml(StringBuilder yamlBuilder, INamedTypeSymbol classSymbol,
            ClassDeclarationSyntax classDeclaration)
        {
            var className = classSymbol.Name;
            
            var fullName = classSymbol.ToDisplayString(s_fullyQualifiedFormat);
            var namespaceName = classSymbol.ContainingNamespace?.ToDisplayString() ?? "<global namespace>";

            yamlBuilder.AppendLine($"  - Name: \"{className}\"");
            yamlBuilder.AppendLine($"    FullName: \"{fullName}\"");
            yamlBuilder.AppendLine($"    Namespace: \"{namespaceName}\"");

            var exportedMembers = GetExportedMembers(classSymbol);
            if (exportedMembers.Count > 0)
            {
                yamlBuilder.AppendLine("    ExportedProperties:");
                foreach (var member in exportedMembers)
                {
                    if (member is IPropertySymbol property)
                    {
                        GeneratePropertyYaml(yamlBuilder, property, classDeclaration);
                    }
                    else if (member is IFieldSymbol field)
                    {
                        GenerateFieldYaml(yamlBuilder, field, classDeclaration);
                    }
                }
            }

            var publicMethods = GetPublicMethods(classSymbol);
            if (publicMethods.Count > 0)
            {
                yamlBuilder.AppendLine("    PublicMethods:");
                foreach (var method in publicMethods)
                {
                    GenerateMethodYaml(yamlBuilder, method);
                }
            }

            var publicStaticMethods = GetPublicStaticMethods(classSymbol);
            if (publicStaticMethods.Count > 0)
            {
                yamlBuilder.AppendLine("    PublicStaticMethods:");
                foreach (var method in publicStaticMethods)
                {
                    GenerateMethodYaml(yamlBuilder, method);
                }
            }

            yamlBuilder.AppendLine();
        }

        private void GenerateFieldYaml(StringBuilder yamlBuilder, IFieldSymbol field,
            ClassDeclarationSyntax classDeclaration)
        {
            var fieldName = field.Name;
            
            var fieldType = field.Type.ToDisplayString(s_fullyQualifiedFormat);

            var defaultValue = GetFieldDefaultValue(field, classDeclaration);
            var eventSignature = GetEventSignature(field.Type);

            yamlBuilder.AppendLine($"      - Name: \"{fieldName}\"");
            yamlBuilder.AppendLine($"        Type: \"{fieldType}\"");

            if (!string.IsNullOrEmpty(defaultValue))
            {
                yamlBuilder.AppendLine($"        DefaultValue: {defaultValue}");
            }

            yamlBuilder.AppendLine($"        CanGet: true");
            yamlBuilder.AppendLine($"        CanSet: true");
            yamlBuilder.AppendLine(
                $"        IsPublic: {(field.DeclaredAccessibility == Accessibility.Public).ToString().ToLower()}");

            if (!string.IsNullOrEmpty(eventSignature))
            {
                yamlBuilder.AppendLine($"        EventSignature: \"{eventSignature}\"");
            }
        }

        private void GeneratePropertyYaml(StringBuilder yamlBuilder, IPropertySymbol property,
            ClassDeclarationSyntax classDeclaration)
        {
            var propertyName = property.Name;
            
            var propertyType = property.Type.ToDisplayString(s_fullyQualifiedFormat);
            var defaultValue = GetPropertyDefaultValue(property, classDeclaration);
            var eventSignature = GetEventSignature(property.Type);

            yamlBuilder.AppendLine($"      - Name: \"{propertyName}\"");
            yamlBuilder.AppendLine($"        Type: \"{propertyType}\"");

            if (!string.IsNullOrEmpty(defaultValue))
            {
                yamlBuilder.AppendLine($"        DefaultValue: {defaultValue}");
            }

            yamlBuilder.AppendLine($"        CanGet: {(property.GetMethod != null).ToString().ToLower()}");
            yamlBuilder.AppendLine($"        CanSet: {(property.SetMethod != null).ToString().ToLower()}");
            yamlBuilder.AppendLine(
                $"        IsPublic: {(property.DeclaredAccessibility == Accessibility.Public).ToString().ToLower()}");


            if (!string.IsNullOrEmpty(eventSignature))
            {
                yamlBuilder.AppendLine($"        EventSignature: \"{eventSignature}\"");
            }
        }

        private void GenerateMethodYaml(StringBuilder yamlBuilder, IMethodSymbol method)
        {
            var methodName = method.Name;
            
            var returnType = method.ReturnType.ToDisplayString(s_fullyQualifiedFormat);
            var signature = GetMethodSignature(method);

            yamlBuilder.AppendLine($"      - Name: \"{methodName}\"");
            yamlBuilder.AppendLine($"        ReturnType: \"{returnType}\"");
            yamlBuilder.AppendLine($"        Signature: \"{signature}\"");
        }

        private string GetMethodSignature(IMethodSymbol method)
        {
            if (method.Parameters.Length == 0)
                return "void";

            
            var parameterTypes = method.Parameters.Select(p => p.Type.ToDisplayString(s_fullyQualifiedFormat));
            return string.Join(",", parameterTypes);
        }

        private string GetEventSignature(ITypeSymbol type)
        {
            var typeName = type.Name;

            if (typeName == "LumaEvent")
            {
                if (type is INamedTypeSymbol namedType && namedType.IsGenericType)
                {
                    var typeArguments = namedType.TypeArguments;
                    if (typeArguments.Length > 0)
                    {
                        
                        var argumentTypes = typeArguments.Select(t => t.ToDisplayString(s_fullyQualifiedFormat));
                        return string.Join(",", argumentTypes);
                    }
                }
                else
                {
                    return "void";
                }
            }

            return "";
        }

        

        private string GetFieldDefaultValue(IFieldSymbol field, ClassDeclarationSyntax classDeclaration)
        {
            var fieldDeclaration = classDeclaration.Members
                .OfType<FieldDeclarationSyntax>()
                .FirstOrDefault(f => f.Declaration.Variables.Any(v => v.Identifier.ValueText == field.Name));

            if (fieldDeclaration?.Declaration.Variables.FirstOrDefault()?.Initializer?.Value != null)
            {
                var initializerText = fieldDeclaration.Declaration.Variables.First().Initializer.Value.ToString();
                return FormatDefaultValueForYaml(initializerText, field.Type);
            }

            return GetTypeDefaultValue(field.Type);
        }

        private string GetPropertyDefaultValue(IPropertySymbol property, ClassDeclarationSyntax classDeclaration)
        {
            var propertyDeclaration = classDeclaration.Members
                .OfType<PropertyDeclarationSyntax>()
                .FirstOrDefault(p => p.Identifier.ValueText == property.Name);

            if (propertyDeclaration?.Initializer?.Value != null)
            {
                var initializerText = propertyDeclaration.Initializer.Value.ToString();
                return FormatDefaultValueForYaml(initializerText, property.Type);
            }

            return GetTypeDefaultValue(property.Type);
        }

        private string FormatDefaultValueForYaml(string value, ITypeSymbol type)
        {
            if (string.IsNullOrEmpty(value))
                return GetTypeDefaultValue(type);

            value = value.TrimEnd('f', 'F', 'd', 'D', 'm', 'M');

            switch (type.SpecialType)
            {
                case SpecialType.System_String:
                    return $"\"{value.Trim('"')}\"";
                case SpecialType.System_Boolean:
                    return value.ToLower();
                case SpecialType.System_Single:
                case SpecialType.System_Double:
                case SpecialType.System_Decimal:
                case SpecialType.System_Int32:
                case SpecialType.System_Int64:
                case SpecialType.System_UInt32:
                case SpecialType.System_UInt64:
                case SpecialType.System_Int16:
                case SpecialType.System_UInt16:
                case SpecialType.System_Byte:
                case SpecialType.System_SByte:
                    return value;
                default:
                    return "null";
            }
        }

        private string GetTypeDefaultValue(ITypeSymbol type)
        {
            switch (type.SpecialType)
            {
                case SpecialType.System_String:
                    return "\"\"";
                case SpecialType.System_Boolean:
                    return "false";
                case SpecialType.System_Single:
                case SpecialType.System_Double:
                case SpecialType.System_Decimal:
                    return "0.0";
                case SpecialType.System_Int32:
                case SpecialType.System_Int64:
                case SpecialType.System_UInt32:
                case SpecialType.System_UInt64:
                case SpecialType.System_Int16:
                case SpecialType.System_UInt16:
                case SpecialType.System_Byte:
                case SpecialType.System_SByte:
                    return "0";
                default:
                    return "null";
            }
        }

        private List<ISymbol> GetExportedMembers(INamedTypeSymbol classSymbol)
        {
            var exportedMembers = new List<ISymbol>();

            foreach (var member in classSymbol.GetMembers())
            {
                var hasExportAttribute = member.GetAttributes()
                    .Any(attr => attr.AttributeClass?.Name == "ExportAttribute");

                if (hasExportAttribute)
                {
                    if (member is IPropertySymbol || member is IFieldSymbol)
                    {
                        exportedMembers.Add(member);
                    }
                }
            }

            return exportedMembers;
        }

        private List<IMethodSymbol> GetPublicMethods(INamedTypeSymbol classSymbol)
        {
            var publicMethods = new List<IMethodSymbol>();

            foreach (var member in classSymbol.GetMembers())
            {
                if (member is IMethodSymbol method &&
                    method.DeclaredAccessibility == Accessibility.Public &&
                    method.MethodKind == MethodKind.Ordinary &&
                    !method.IsStatic &&
                    !IsOverriddenBaseMethod(method))
                {
                    publicMethods.Add(method);
                }
            }

            return publicMethods;
        }

        private List<IMethodSymbol> GetPublicStaticMethods(INamedTypeSymbol classSymbol)
        {
            var publicStaticMethods = new List<IMethodSymbol>();

            foreach (var member in classSymbol.GetMembers())
            {
                if (member is IMethodSymbol method &&
                    method.DeclaredAccessibility == Accessibility.Public &&
                    method.MethodKind == MethodKind.Ordinary &&
                    method.IsStatic)
                {
                    publicStaticMethods.Add(method);
                }
            }

            return publicStaticMethods;
        }

        private bool IsOverriddenBaseMethod(IMethodSymbol method)
        {
            var baseMethodNames = new[] { "OnCreate", "OnUpdate", "OnDestroy", "OnEnable", "OnDisable" };
            return baseMethodNames.Contains(method.Name) && method.IsOverride;
        }

        private bool IsScriptClass(INamedTypeSymbol classSymbol)
        {
            var baseType = classSymbol.BaseType;
            while (baseType != null)
            {
                if (baseType.Name == "Script")
                {
                    return true;
                }

                baseType = baseType.BaseType;
            }

            return false;
        }
    }

    internal class ScriptSyntaxReceiver : ISyntaxReceiver
    {
        public List<ClassDeclarationSyntax> CandidateClasses { get; } = new List<ClassDeclarationSyntax>();

        public void OnVisitSyntaxNode(SyntaxNode syntaxNode)
        {
            if (syntaxNode is ClassDeclarationSyntax classDeclaration &&
                classDeclaration.BaseList != null)
            {
                CandidateClasses.Add(classDeclaration);
            }
        }
    }
}