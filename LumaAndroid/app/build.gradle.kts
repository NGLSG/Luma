plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

android {
    namespace = "com.lumaengine.lumaandroid"
    compileSdk = 36
    ndkVersion = "27.0.12077973"

    defaultConfig {
        applicationId = "com.lumaengine.lumaandroid"
        minSdk = 28
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        // 仅构建 arm64-v8a
        ndk {
            abiFilters += listOf("arm64-v8a")
        }

        // 传递 CMake 参数
        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_ABI=arm64-v8a",
                    "-DANDROID_PLATFORM=android-28",
                    "-DCMAKE_SYSTEM_NAME=Android",
                    "-DCMAKE_SUPPRESS_DEVELOPER_WARNINGS=TRUE",
                    "-DCMAKE_TOOLCHAIN_FILE=E:/vcpkg/scripts/buildsystems/vcpkg.cmake",
                    "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=E:/Android/Sdk/ndk/27.0.12077973/build/cmake/android.toolchain.cmake",
                    "-DVCPKG_TARGET_TRIPLET=arm64-android",
                    "-DUSE_PREBUILT_ENGINE=OFF",
                    "-DENABLE_LIGHTWEIGHT_BUILD=ON"  // 启用轻量化构建
                )
                cFlags += listOf("-v")
                cppFlags += listOf("-v", "-std=c++20")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
        debug {
            isDebuggable = true
            isJniDebuggable = true
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    kotlinOptions {
        jvmTarget = "11"
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildFeatures {
        viewBinding = true
    }

    // AGP 8+ 打包配置
    packaging {
        resources {
            excludes += setOf("META-INF/**")
        }
        jniLibs {
            // 改为 true 以确保 Mono 共享库被正确打包
            useLegacyPackaging = true
            // 轻量化构建时不保留调试符号
            // keepDebugSymbols += listOf("**/*.so")
        }
    }

    // 不压缩的资源文件
    androidResources {
        noCompress += listOf("luma_pack", "lproj", "dll", "json", "manifest", "jar")
    }

    // 源码集配置
    sourceSets {
        getByName("main") {
            jniLibs.setSrcDirs(listOf("src/main/jniLibs"))
            assets.setSrcDirs(listOf("src/main/assets"))
            java.srcDirs("src/main/java")
        }
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.constraintlayout)

    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
}