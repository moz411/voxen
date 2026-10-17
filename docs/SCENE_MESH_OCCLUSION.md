# Scene Mesh / Occlusion — étape 1

Branche : `feature/scene-mesh-occlusion`, issue de `main`.

## Implémenté
- Active `XR_META_spatial_entity_mesh` quand l'extension est exposée par le runtime Meta.
- Charge `xrGetSpaceTriangleMeshMETA` dynamiquement.
- Pour chaque entité découverte, demande `XR_SPACE_COMPONENT_TYPE_TRIANGLE_MESH_META` de manière asynchrone, en conservant le mécanisme anti-boucle existant.
- Quand un composant mesh est activé, interroge une fois les nombres de sommets et d'indices ; logs sous `VoxenScene`.
- Les sommets/indices sont désormais acquis sur le Quest, et un rendu Vulkan en lignes (triangle edges) est disponible dans BACKGROUNDS > **Scene mesh wireframe**. Le buffer GPU est construit une seule fois, les poses sont actualisées à chaque frame. Première version : seul le premier mesh capturé est affiché. **L'occlusion n'est pas encore implémentée.**

## Vérification Quest 3
```bash
cd ~/voxen
git fetch origin
git switch feature/scene-mesh-occlusion
git pull --ff-only
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop org.voxen.xr
adb logcat -c
adb shell monkey -p org.voxen.xr 1
sleep 5
adb logcat -d -s VoxenScene:I AndroidRuntime:E
```

Attendre `Triangle mesh function available: 1`, puis `Triangle mesh vertex count` et `Triangle mesh index count` pour au moins une entité. Un composant peut être absent ou son activation rejetée : ce cas ne doit pas faire planter l'app.

## Séquence prévue
1. **Fait :** récupérer et valider les sommets et indices (17 523 sommets / 35 242 triangles sur Quest 3).
2. **À tester :** afficher le maillage en filaire vert avec l'option BACKGROUNDS > Scene mesh wireframe. Vérifier alignement stéréo et charge GPU.
3. Étendre le diagnostic à plusieurs ancres, puis ajouter un pass profondeur statique pour l'occlusion par géométrie enregistrée.
4. Étudier `XR_META_environment_depth` pour les objets déplacés et l'occlusion dynamique.
5. Validation stéréo, coût GPU et sécurité avant fusion dans main.
