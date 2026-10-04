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
            }
        }
        ndk { abiFilters += "arm64-v8a" }
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "3.22.1"
        }
    }
}
