using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System.Diagnostics;




public static class Program
{
    
    
    
    
    public static void Main(string[] args)
    {
        if (args.Length == 0 || string.IsNullOrWhiteSpace(args[0]))
        {
            Console.Error.WriteLine("Error: Missing file path argument.");
            return;
        }

        string filePath = args[0];
        if (!File.Exists(filePath))
        {
            Console.Error.WriteLine($"Error: File not found at '{filePath}'.");
            return;
        }

        try
        {
            string? fullClassName = GetScriptClassName(filePath);
            if (!string.IsNullOrEmpty(fullClassName))
            {
                
                Console.WriteLine(fullClassName);
            }
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"An unexpected error occurred: {ex.Message}");
        }
    }

    
    
    
    
    
    private static string? GetScriptClassName(string filePath)
    {
        string fileContent = File.ReadAllText(filePath);
        SyntaxTree syntaxTree = CSharpSyntaxTree.ParseText(fileContent);
        CompilationUnitSyntax root = syntaxTree.GetCompilationUnitRoot();

        
        var classDeclarations = root.DescendantNodes().OfType<ClassDeclarationSyntax>();

        foreach (var classDecl in classDeclarations)
        {
            
            if (classDecl.BaseList == null)
            {
                continue;
            }

            bool inheritsFromScript = classDecl.BaseList.Types.Any(baseType =>
                baseType.Type is IdentifierNameSyntax id && id.Identifier.Text == "Script");

            if (inheritsFromScript)
            {
                string className = classDecl.Identifier.Text;
                string? namespaceName = GetNamespace(classDecl);

                return string.IsNullOrEmpty(namespaceName) ? className : $"{namespaceName}.{className}";
            }
        }

        return null;
    }

    
    
    
    
    
    private static string? GetNamespace(SyntaxNode node)
    {
        
        var potentialNamespace = node.Parent;
        while (potentialNamespace != null &&
               !(potentialNamespace is NamespaceDeclarationSyntax) &&
               !(potentialNamespace is FileScopedNamespaceDeclarationSyntax))
        {
            potentialNamespace = potentialNamespace.Parent;
        }

        if (potentialNamespace is BaseNamespaceDeclarationSyntax namespaceDecl)
        {
            return namespaceDecl.Name.ToString();
        }

        return null;
    }
}