#include "Shader.h"

SKSLShader::SKSLShader() = default;


SKSLShader SKSLShader::FromCode(const std::string& vertexSksl, const std::string& fragmentSksl)
{
    const std::string combinedSksl = vertexSksl + "\n" + fragmentSksl;


    auto [effect, error] = SkRuntimeEffect::MakeForShader(
        SkString(combinedSksl.c_str()),
        SkRuntimeEffect::Options{}
    );

    if (!effect)
    {
        std::cerr << "Shader pipeline compilation failed (using MakeForShader):\n" << error.c_str() << std::endl;
        return {};
    }
    return SKSLShader(std::move(effect));
}

SKSLShader SKSLShader::FromFile(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
{
    if (!std::filesystem::exists(vertexPath))
    {
        std::cerr << "Vertex shader file not found: " << vertexPath << std::endl;
        return {};
    }
    if (!std::filesystem::exists(fragmentPath))
    {
        std::cerr << "Fragment shader file not found: " << fragmentPath << std::endl;
        return {};
    }

    std::unordered_set<std::filesystem::path, PathHash> visitedVs;
    const std::string vsCode = PreprocessSksl(vertexPath, visitedVs);
    if (vsCode.empty()) return {};

    std::unordered_set<std::filesystem::path, PathHash> visitedFs;
    const std::string fsCode = PreprocessSksl(fragmentPath, visitedFs);
    if (fsCode.empty()) return {};

    return FromCode(vsCode, fsCode);
}


SKSLShader SKSLShader::FromFragmentCode(const std::string& fragmentSksl)
{
    auto [effect, error] = SkRuntimeEffect::MakeForShader(SkString(fragmentSksl.c_str()));
    if (!effect)
    {
        std::cerr << "Fragment shader compilation failed:\n" << error.c_str() << std::endl;
        return {};
    }
    return SKSLShader(std::move(effect));
}

SKSLShader SKSLShader::FromFragmentFile(const std::filesystem::path& fragmentPath)
{
    if (!std::filesystem::exists(fragmentPath))
    {
        std::cerr << "Shader file not found: " << fragmentPath << std::endl;
        return {};
    }

    std::unordered_set<std::filesystem::path, PathHash> visitedFiles;
    const std::string processedCode = PreprocessSksl(fragmentPath, visitedFiles);

    if (processedCode.empty())
    {
        std::cerr << "Failed to process shader file or file is empty: " << fragmentPath << std::endl;
        return {};
    }

    return FromFragmentCode(processedCode);
}


SKSLShader SKSLShader::FromCode(const std::string& skslCode)
{
    return FromFragmentCode(skslCode);
}

SKSLShader SKSLShader::FromFile(const std::filesystem::path& filePath)
{
    return FromFragmentFile(filePath);
}


bool SKSLShader::IsValid() const
{
    return effect != nullptr;
}

SKSLShader::operator sk_sp<SkRuntimeEffect>() const
{
    return effect;
}

SKSLShader::SKSLShader(sk_sp<SkRuntimeEffect> runtimeEffect) : effect(std::move(runtimeEffect))
{
}

std::string SKSLShader::PreprocessSksl(const std::filesystem::path& filePath,
                                   std::unordered_set<std::filesystem::path, PathHash>& visited)
{
    const auto canonicalPath = std::filesystem::canonical(filePath);
    if (visited.count(canonicalPath))
    {
        return "";
    }
    visited.insert(canonicalPath);


    std::ifstream fileStream(filePath);
    if (!fileStream.is_open())
    {
        std::cerr << "Failed to open shader include file: " << filePath << std::endl;
        return "";
    }

    std::stringstream output;
    std::string line;
    const std::string includeDirective = "//@include";
    const auto parentPath = filePath.parent_path();


    while (std::getline(fileStream, line))
    {
        size_t firstChar = line.find_first_not_of(" \t");
        if (firstChar != std::string::npos && line.rfind(includeDirective, firstChar) == firstChar)
        {
            size_t firstQuote = line.find('"', firstChar);
            size_t lastQuote = line.find('"', firstQuote + 1);
            if (firstQuote != std::string::npos && lastQuote != std::string::npos)
            {
                std::string includePathStr = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                std::filesystem::path includePath = parentPath / includePathStr;


                output << PreprocessSksl(includePath, visited) << "\n";
            }
        }
        else
        {
            output << line << "\n";
        }
    }

    return output.str();
}
