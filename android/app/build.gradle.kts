val voxenStereoKit = providers.gradleProperty("voxenStereoKit").orNull?.toBoolean() ?: true

plugins { id("com.android.application") }

android {
    namespace = "org.voxen.xr"
    compileSdk = 35

    defaultConfig {
        applicationId = "org.voxen.xr"
        minSdk = 29
        targetSdk = 35
        versionCode = 1
        versionName = "0.1.0"

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
                arguments += "-DANDROID_STL=c++_shared"
                arguments += "-DVOXEN_USE_STEREOKIT=${if (voxenStereoKit) "ON" else "OFF"}"
            }
        }
        ndk { abiFilters += "arm64-v8a" }
    }

    sourceSets.getByName("main").manifest.srcFile(
        if (voxenStereoKit) "src/stereokit/AndroidManifest.xml" else "src/main/AndroidManifest.xml"
    )

    buildFeatures {
        prefab = true
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

dependencies {
    implementation("org.khronos.openxr:openxr_loader_for_android:1.1.63")
}
