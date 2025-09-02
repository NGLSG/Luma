using System;
using System.IO;
using System.Reflection;

namespace Luma.Tools
{
    
    
    
    class YamlExtractor
    {
        static void Main(string[] args)
        {
            try
            {
                string assemblyPath = args.Length > 0 ? args[0] : "GameScripts.dll";
                string outputPath = args.Length > 1 ? args[1] : "ScriptMetadata.yaml";

                Console.WriteLine($"正在加载程序集: {assemblyPath}");
                
                if (!File.Exists(assemblyPath))
                {
                    Console.WriteLine($"错误: 找不到程序集文件 {assemblyPath}");
                    return;
                }

                
                var assembly = Assembly.LoadFrom(assemblyPath);
                
                
                var metadataType = assembly.GetType("Luma.Generated.ScriptMetadata");
                if (metadataType == null)
                {
                    Console.WriteLine("警告: 未找到生成的脚本元数据，可能没有继承自Script的类");
                    CreateEmptyYamlFile(outputPath);
                    return;
                }

                
                var writeToFileMethod = metadataType.GetMethod("WriteToFile", BindingFlags.Public | BindingFlags.Static);
                if (writeToFileMethod != null)
                {
                    writeToFileMethod.Invoke(null, new object[] { outputPath });
                    Console.WriteLine($"成功生成YAML文件: {outputPath}");
                }
                else
                {
                    
                    var yamlContentField = metadataType.GetField("YamlContent", BindingFlags.Public | BindingFlags.Static);
                    if (yamlContentField == null)
                    {
                        Console.WriteLine("错误: 未找到YamlContent字段或WriteToFile方法");
                        return;
                    }

                    var yamlContent = yamlContentField.GetValue(null) as string;
                    if (string.IsNullOrEmpty(yamlContent))
                    {
                        Console.WriteLine("警告: YAML内容为空");
                        CreateEmptyYamlFile(outputPath);
                        return;
                    }

                    
                    File.WriteAllText(outputPath, yamlContent);
                    Console.WriteLine($"成功生成YAML文件: {outputPath}");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"错误: {ex.Message}");
                Console.WriteLine($"堆栈跟踪: {ex.StackTrace}");
                Environment.Exit(1);
            }
        }

        private static void CreateEmptyYamlFile(string outputPath)
        {
            var emptyYaml = @"# Luma引擎脚本元数据配置文件
# 此文件由Luma.SourceGenerator自动生成，请勿手动修改

Scripts:
  # 未发现继承自Script的类
";
            File.WriteAllText(outputPath, emptyYaml);
            Console.WriteLine($"创建空的YAML文件: {outputPath}");
        }
    }
}