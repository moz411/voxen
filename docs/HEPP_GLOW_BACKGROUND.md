# Hepp Glow — attributed background shader

Source code supplied by the user, credit:
- Created by **Hepp Maccoy (2019)** — hepp@audiopixel.com — http://audiopixel.com
- Distance functions credited to **Inigo Quilez** — https://iquilezles.org
- Original declared license: **Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0)**

This Vulkan GLSL adaptation is a modified work: the Shadertoy mainImage/iTime/iResolution interface was adapted to the engine's shader interface (wall UV, time, animation speed, opacity), and the raymarch iteration budget was reduced from 50 to 20 for Meta Quest 3. Full credit and original license are retained in the shader file.

**Important distribution caveat:** CC BY-NC-SA is noncommercial and share-alike. Do not merge/distribute as if it were covered only by the repository's normal software license; verify compatibility and comply with the original license terms before commercial distribution. 

Test in BACKGROUNDS > HEPP GLOW. Start with Max walls = 1 and opacity <= 0.4, then scale up; observe GPU time. Static Scene Mesh occlusion remains compatible.
