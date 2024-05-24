# A copy of the art repository for my build of Pixel OS.

Mainly used for: To restore compatibility for some popular obfuscated Chinese apps that use a specific obfuscation SDK and rely on private APIs, you can disable stripping of libart library symbols. These apps have compatibility issues in Android 14 QPR2 because they do not use the mainline ART module based on old code (such as the stock Pixel OS).

I regularly pick commits from the aosp art repository to this copy
