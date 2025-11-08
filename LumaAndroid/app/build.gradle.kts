import java.util.Properties

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

val gradleProps = Properties().apply {
    val propsFile = rootProject.file("gradle.properties")
    if (propsFile.exists()) {
        propsFile.inputStream().use { load(it) }
    }
}

fun prop(key: String, defaultValue: String) = gradleProps.getProperty(key, defaultValue)
fun propInt(key: String, defaultValue: String) = prop(key, defaultValue).toInt()
fun propBool(key: String, defaultValue: String) = prop(key, defaultValue).toBoolean()
fun propList(key: String): List<String> =
    gradleProps.getProperty(key)?.split(",")?.map { it.trim() }?.filter { it.isNotEmpty() } ?: emptyList()

val appNamespace = prop("appNamespace", "com.lumaengine.lumaandroid")
val applicationIdValue = prop("applicationId", appNamespace)
val minVersionValue = prop("minVersion", prop("minSdk", "28"))
val maxVersionValue = prop("maxVersion", prop("targetSdk", "36"))
val signingStoreFile = prop("signingStoreFile", "")
val signingStorePassword = prop("signingStorePassword", "")
val signingKeyAlias = prop("signingKeyAlias", "")
val signingKeyPassword = prop("signingKeyPassword", "")

android {
    namespace = appNamespace
    compileSdk = propInt("compileSdk", maxVersionValue)
    ndkVersion = prop("ndkVersion", "27.0.12077973")

    defaultConfig {
        applicationId = applicationIdValue
        minSdk = propInt("minSdk", minVersionValue)
        targetSdk = propInt("targetSdk", maxVersionValue)
        versionCode = propInt("versionCode", "1")
        versionName = prop("versionName", "1.0")

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            val abiFiltersValues = propList("abiFilters")
            if (abiFiltersValues.isNotEmpty()) {
                abiFilters.clear()
                abiFilters.addAll(abiFiltersValues)
            }
        }

        externalNativeBuild {
            cmake {
                val cmakeArgsValues = propList("cmakeArgs")
                if (cmakeArgsValues.isNotEmpty()) {
                    arguments.clear()
                    arguments.addAll(cmakeArgsValues)
                }

                val cFlagsValues = propList("cFlags")
                if (cFlagsValues.isNotEmpty()) {
                    cFlags.clear()
                    cFlags.addAll(cFlagsValues)
                }

                val cppFlagsValues = propList("cppFlags")
                if (cppFlagsValues.isNotEmpty()) {
                    cppFlags.clear()
                    cppFlags.addAll(cppFlagsValues)
                }
            }
        }
    }

    val releaseSigningConfig = if (signingStoreFile.isNotBlank() && signingKeyAlias.isNotBlank()) {
        signingConfigs.create("release").apply {
            storeFile = file(signingStoreFile)
            storePassword = signingStorePassword
            keyAlias = signingKeyAlias
            keyPassword = signingKeyPassword
            enableV1Signing = true
            enableV2Signing = true
            enableV3Signing = true
            enableV4Signing = true
        }
    } else null

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            if (releaseSigningConfig != null) {
                signingConfig = releaseSigningConfig
            } else {
                println("Warning: 未配置签名信息，Release APK 将不会签名。")
            }
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
            version = prop("cmakeVersion", "3.22.1")
        }
    }

    buildFeatures {
        viewBinding = true
    }

    packaging {
        resources {
            excludes += setOf("META-INF/**")
        }
        jniLibs {
            useLegacyPackaging = propBool("useLegacyJniPacking", "true")
        }
    }

    androidResources {
        noCompress += listOf("luma_pack", "lproj", "dll", "json", "manifest", "jar")
    }

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
